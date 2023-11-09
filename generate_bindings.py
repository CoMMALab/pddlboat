import argparse
import re
import subprocess
from collections import defaultdict
from dataclasses import dataclass
from itertools import repeat
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Set, Tuple

import clang.cindex
from clang.cindex import (
    AccessSpecifier,
    AvailabilityKind,
    Cursor,
    CursorKind,
    Index,
    SourceLocation,
    TranslationUnit,
    Type,
    TypeKind,
    os
)

METHOD_REPLACEMENTS = {}
SUBMODULE_NAMESPACES = []


def get_header_directories() -> List[str]:
  p = subprocess.run(["gcc", "-xc++", "-E", "-v", "/dev/null"], capture_output=True, text=True)
  start_token = "#include <...> search starts here:"
  start = p.stderr.find(start_token)
  end = p.stderr.find("End of search list.")
  return [
      "-I" + line.strip() for line in p.stderr[start + len(start_token) + 1:end - 1].split('\n')
  ]


def format_extent(extent) -> str:
  start_loc: SourceLocation = extent.start
  return f'{start_loc.file.name}: line {start_loc.line}'  # type: ignore


def fix_paths(comp_db_dir: Path, exec_dir: Path, arg: str) -> str:
  # TODO: Make this less brittle
  if arg[:2] != '-I' and (arg[0] == '-' or arg == 'c++-header'):
    return arg
  elif arg[:3] == '-I/' or Path(arg).is_absolute():
    return arg

  if arg[:2] == '-I':
    path = comp_db_dir / Path(arg[2:])
    return f'-I{path.relative_to(exec_dir)}'
  else:
    path = comp_db_dir / Path(arg)
    return str(path.relative_to(exec_dir))


def load_data(compilation_database_dir: Path,
              header_paths: List[Path]) -> Tuple[Index, List[TranslationUnit]]:
  # Parse the given input file
  index = clang.cindex.Index.create()
  compilation_database_dir = compilation_database_dir.absolute()
  comp_db = clang.cindex.CompilationDatabase.fromDirectory(compilation_database_dir)
  translation_units = []
  cwd = Path(os.getcwd())
  for header_path in header_paths:
    commands = comp_db.getCompileCommands(header_path)
    file_args = []
    for command in commands:
      file_args.extend(command.arguments)

    file_args = file_args[2:-2]
    file_args = list(map(lambda arg: fix_paths(compilation_database_dir, cwd, arg), file_args))
    # NOTE: Not sure why this needs to be manually included, but we don't find stddef otherwise
    # ZK: Added more general way by querying the compiler for default include directories
    # file_args.append('-I/usr/lib/clang/14.0.6/include')
    file_args.extend(get_header_directories())

    # if 'build/c-header' in file_args:
    #   file_args[file_args.index('build/c-header')] = 'build/c++-header'

    file_args = [arg for arg in file_args if '-I' == arg[:2] or '-std' == arg[:4]]
    # file_args.append('-I/home/zak/code/pddlboat/include')
    # for invalid in ['-fext-numeric-literals', '-fconcepts', '-fno-fat-lto-objects']:
    #   if invalid in file_args:
    #     file_args.remove(invalid)

    # mq_idx = file_args.index('-MQ')
    # mf_idx = file_args.index('-MF')
    # file_args = file_args[:mq_idx] + file_args[mq_idx + 2:mf_idx] + file_args[mf_idx + 2:]

    print(file_args)

    try:
      translation_units.append(
          index.parse(header_path, file_args, options=TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
      )
    except Exception as e:
      print(header_path, file_args)
      raise e

  return index, translation_units


def get_nodes_from_file(nodes: Iterable[Cursor], file_name: str) -> List[Cursor]:
  return list(filter(lambda n: n.location.file.name == file_name, nodes))


def get_nodes_with_kind(nodes: Iterable[Cursor], node_kinds: Iterable[CursorKind]) -> List[Cursor]:
  return list(filter(lambda n: n.kind in node_kinds, nodes))


# This approach to snake-case conversion adapted from:
# https://stackoverflow.com/questions/1175208/elegant-python-function-to-convert-camelcase-to-snake-case
SPECIAL_KEYWORDS = re.compile(r'ID|YAML|SRDF|URDF|JSON|IK|PDDL')
NAIVE_CAMEL_CASE = re.compile(r'(.)([A-Z][a-z]+)')
CAMEL_CASE_UNDERSCORES = re.compile(r'__([A-Z])')
ADVANCED_CAMEL_CASE = re.compile(r'([a-z0-9])([A-Z])')


def downcase_keywords(kwd_match: re.Match) -> str:
  kwd = kwd_match.group(0)
  return kwd[0] + kwd[1:].lower()


def to_snake_case(camel_name: str) -> str:
  snake_name = SPECIAL_KEYWORDS.sub(downcase_keywords, camel_name)
  snake_name = NAIVE_CAMEL_CASE.sub(r'\1_\2', snake_name)
  snake_name = CAMEL_CASE_UNDERSCORES.sub(r'_\1', snake_name)
  snake_name = ADVANCED_CAMEL_CASE.sub(r'\1_\2', snake_name)
  return snake_name.lower()


def generate_function_pointer_signature(
    qualified_name: str,
    function_node: Cursor,
    is_static: bool = False,
    class_node: Optional[Cursor] = None
) -> str:
  signature = f'{function_node.type.get_result().get_canonical().spelling} ({qualified_name + "::*" if class_node and not is_static else "*"})('
  arg_types = function_node.type.argument_types()
  for typ in function_node.type.argument_types():
    signature += f'{typ.get_canonical().spelling}, '

  if len(arg_types) > 0:
    signature = signature[:-2]

  signature += ')'

  if function_node.is_const_method():
    signature += ' const'

  return signature


def generate_overloads(
    name: str,
    nodes: Iterable[Cursor],
    qualified_name: str,
    class_node: Optional[Cursor] = None
) -> List[str]:
  overloads = []
  for function_node in nodes:
    is_static = function_node.is_static_method()
    function_pointer_signature = generate_function_pointer_signature(
        qualified_name, function_node, is_static, class_node
    )
    if is_static:
      # NOTE: This is a gross hack but seems necessary given limitations of pybind11
      overload_name = f'{name}_static'
    else:
      overload_name = name

    overloads.append(
        f'{".def_static" if is_static else ".def"}("{overload_name}", static_cast<{function_pointer_signature}>(&{qualified_name}::{function_node.spelling}))'
    )

  return overloads


def get_base_type_name(typ: Type) -> str:
  type_name_chunks = typ.get_canonical().spelling.split(' ')
  # Check for const and reference
  return ' '.join(chunk if chunk not in ['const', '&'] else '' for chunk in type_name_chunks
                 ).strip()


def generate_operator_methods(operator: str, nodes: Iterable[Cursor],
                              qualified_name: str) -> List[str]:
  methods = []
  for method_node in nodes:
    argument_type_names = [get_base_type_name(typ) for typ in method_node.type.argument_types()]
    if len(argument_type_names) >= 2:
      print(method_node.spelling, argument_type_names)

    assert (len(argument_type_names) < 2)
    if len(argument_type_names) == 0:
      methods.append(f'.def({operator}py::self)')
    elif operator == '[]':
      methods.append(
          f'.def("__getitem__", []({qualified_name} &s, {argument_type_names[0]} i) {{ return s[i]; }})'
      )
    else:
      methods.append(
          f'.def(py::self {operator} {"py::self" if argument_type_names[0] == qualified_name else argument_type_names[0] + "()"})'
      )

  return methods


def is_exposed(node: Cursor) -> bool:
  return node.access_specifier == AccessSpecifier.PUBLIC  # type: ignore


def is_not_private(node: Cursor) -> bool:
  return node.access_specifier != AccessSpecifier.PRIVATE  # type: ignore


# TODO: This could be more efficient if we called get_children once per class and iterated over it
# multiple times, at the cost of threading that list through in the parameters
# TODO: Could also improve efficiency with stronger preference for iterators over explicit lists


def get_exposed_fields(class_node: Cursor) -> List[Cursor]:
  fields = get_nodes_with_kind(class_node.get_children(), [CursorKind.FIELD_DECL])  # type: ignore
  return list(filter(is_exposed, fields))


def get_exposed_static_variables(class_node: Cursor) -> List[Cursor]:
  fields = get_nodes_with_kind(class_node.get_children(), [CursorKind.VAR_DECL])  # type: ignore
  return list(filter(is_exposed, fields))


def get_exposed_methods(class_node: Cursor) -> List[Cursor]:
  methods = get_nodes_with_kind(class_node.get_children(), [CursorKind.CXX_METHOD])  # type: ignore
  return list(filter(is_exposed, methods))


# TODO: Handle default arguments
# TODO: Maybe generate keyword args?
# TODO: Maybe export constants?


@dataclass
class Binding:
  name: str
  is_class: bool
  dependencies: List[str]
  body: List[str]
  external_parent: Optional[str] = None

  def __repr__(self) -> str:
    return f'Binding(name={self.name}, is_class={self.is_class}, dependencies={self.dependencies}, external={self.external_parent})'


def generate_constructor_wrapper(qualified_name: str, argument_types: List[Type]) -> str:
  '''Generate an anonymous wrapper for constructors taking double-pointer arguments.'''
  # TODO: Could generalize this to wrap functions with double-pointer arguments, but I haven't found
  # TODO: This whole function should be rewritten; it's messy and inefficient
  # any in the codebase yet
  modified_arg_types = []
  modified_arg_indices = set()
  for i, typ in enumerate(argument_types):
    canonical_typ = typ.get_canonical()
    pointee_type = canonical_typ.get_pointee().get_pointee()
    if pointee_type.kind != TypeKind.INVALID:  # type: ignore
      # See pybind11 known limitation #1
      if pointee_type.kind in (
          TypeKind.CHAR_U,  # type: ignore
          TypeKind.UCHAR):  # type: ignore
        vec_elem_type = 'std::string'
      else:
        vec_elem_type = canonical_typ.get_pointee().spelling

      modified_arg_types.append(f'const std::vector<{vec_elem_type}>&')
      modified_arg_indices.add(i)
    else:
      modified_arg_types.append(canonical_typ.spelling)

  arg_names = [f'arg_{i}' for i in range(len(modified_arg_types))]
  lambda_args = [typ + ' ' + arg_name for typ, arg_name in zip(modified_arg_types, arg_names)]

  invocation_expr = ', '.join(
      arg_name if i not in modified_arg_indices else f'convertVec({arg_name}).data()' for i,
      arg_name in enumerate(arg_names)
  )

  return f'.def(py::init([]({", ".join(lambda_args)}) {{return std::make_shared<{qualified_name}>({invocation_expr});}}))'


def generate_constructors(class_node: Cursor, qualified_name: str) -> List[str]:
  constructors = []
  for constructor_node in get_nodes_with_kind(class_node.get_children(),
                                              [CursorKind.CONSTRUCTOR]):  # type: ignore
    if is_not_private(
        constructor_node
    ) and constructor_node.availability != AvailabilityKind.NOT_AVAILABLE:  # type: ignore
      # TODO: This could be cleaner
      if any(typ.get_pointee().get_pointee().kind != TypeKind.INVALID  # type: ignore
             for typ in constructor_node.type.argument_types()):
        constructors.append(
            generate_constructor_wrapper(qualified_name, constructor_node.type.argument_types())
        )
      else:
        constructor_type_string = ''
        constructor_argument_types = [
            typ.get_canonical().spelling for typ in constructor_node.type.argument_types()
        ]
        if constructor_argument_types:
          constructor_type_string = f'<{", ".join(constructor_argument_types)}>'
        constructors.append(f".def(py::init{constructor_type_string}())")

  return constructors


def generate_methods(class_node: Cursor, qualified_name: str) -> List[str]:
  class_method_nodes = defaultdict(list)
  for method_node in get_exposed_methods(class_node):
    class_method_nodes[to_snake_case(method_node.spelling)].append(method_node)

  methods = []
  for method_name, method_nodes in class_method_nodes.items():
    # Handle operators
    # if 'operator' in method_name:
    #   print(method_name)
    if method_name[:8] == 'operator':
      operator = method_name[8:].strip()
      if operator[:3] == 'new' or operator[:6] == 'delete' or operator == '=':
        continue

      methods.extend(generate_operator_methods(operator, method_nodes, qualified_name))
    elif len(method_nodes) > 1:
      methods.extend(generate_overloads(method_name, method_nodes, qualified_name, class_node))

    elif method_name in METHOD_REPLACEMENTS:
      repl, func = METHOD_REPLACEMENTS[method_name]
      methods.append(f'.def("{repl}", {func.format(qualified_name = qualified_name)})')

    else:
      methods.append(
          f'{".def_static" if method_nodes[0].is_static_method() else ".def"}("{method_name}", &{qualified_name}::{method_nodes[0].spelling})'
      )

  return methods


def generate_fields(class_node: Cursor, qualified_name: str) -> List[str]:
  fields = []
  # Instance fields
  for field_node in get_exposed_fields(class_node):
    field_binder = '.def_readwrite'
    if field_node.type.is_const_qualified():
      field_binder = '.def_readonly'

    fields.append(
        f'{field_binder}("{to_snake_case(field_node.spelling)}", &{qualified_name}::{field_node.spelling})'
    )

  # Static variables
  for static_var_node in get_exposed_static_variables(class_node):
    var_binder = '.def_readwrite'
    if static_var_node.type.is_const_qualified():
      var_binder = '.def_readonly_static'

    fields.append(
        f'{var_binder}("{to_snake_case(static_var_node.spelling)}", &{qualified_name}::{static_var_node.spelling})'
    )

  return fields


def get_nested_types(class_node: Cursor) -> List[Cursor]:
  return get_nodes_with_kind(
      class_node.get_children(), [CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL]
  )  # type: ignore


def get_nested_enums(class_node: Cursor) -> list[Cursor]:
  return get_nodes_with_kind(class_node.get_children(), [CursorKind.ENUM_DECL])


def generate_enum(
    enum_node: Cursor,
    ns_name: str,
    parent_class: Optional[str] = None,
    submodule_name: Optional[str] = None
) -> List[Binding]:
  # Handle forward declarations
  enum_definition_node = enum_node.get_definition()
  qualified_name = f'{parent_class if parent_class else ns_name}::{enum_node.spelling}'
  if enum_definition_node is None or enum_definition_node != enum_node:
    print(
        f'{qualified_name} appears to be a forward declaration at {format_extent(enum_node.extent)}'
    )
    return []

  if parent_class:
    parent_object = f'py_{parent_class.replace("::", "_")}'
  elif submodule_name:
    parent_object = f"{submodule_name}_m"
  else:
    parent_object = "m"

  print(f'Processing {qualified_name} at {format_extent(enum_node.extent)}...')
  # NOTE: We assume that everything uses shared_ptr as its holder type for simplicity
  enum_output = [f'// Bindings for enum {qualified_name}']
  enum_output.append(f'py::enum_<{qualified_name}>({parent_object}, "{enum_node.spelling}")')
  # Enum values
  for ec in enum_definition_node.get_children():
    enum_output.append(f'.value("{ec.spelling}", {qualified_name}::{ec.spelling})')

  enum_output.append(".export_values()")
  enum_output.append(';')
  enum_binding = Binding(qualified_name, True, dependencies=[], body=enum_output)
  if parent_class:
    enum_binding.dependencies.append(parent_class)

  return [enum_binding]


def generate_class(
    class_node: Cursor,
    ns_name: str,
    pointer_names: Set[str],
    parent_class: Optional[str] = None,
    submodule_name: Optional[str] = None
) -> List[Binding]:
  # Handle forward declarations
  class_definition_node = class_node.get_definition()
  qualified_name = f'{parent_class if parent_class else ns_name}::{class_node.spelling}'
  if class_definition_node is None or class_definition_node != class_node:
    print(
        f'{qualified_name} appears to be a forward declaration at {format_extent(class_node.extent)}'
    )
    return []

  if parent_class:
    parent_object = f'py_{parent_class.replace("::", "_")}'
  elif submodule_name:
    parent_object = f"{submodule_name}_m"
  else:
    parent_object = "m"

  superclasses = [
      sc_node.type.get_canonical().spelling for sc_node in
      get_nodes_with_kind(class_node.get_children(), [CursorKind.CXX_BASE_SPECIFIER]) # type: ignore
  ]
  superclass_string = ''
  print(f'Processing {qualified_name} at {format_extent(class_node.extent)}...')
  # NOTE: We assume that everything uses shared_ptr as its holder type for simplicity
  pointer_string = f', std::shared_ptr<{qualified_name}>'
  class_output = [f'// Bindings for class {qualified_name}']
  filtered_superclasses = [superclass for superclass in superclasses if superclass[:5] != 'std::']
  if filtered_superclasses:
    superclass_string = f', {",".join(filtered_superclasses)}'

  nested_output: List[Binding] = []
  if 'std::exception' in superclasses:
    class_output.append(
        f'py::register_exception<{qualified_name}>({parent_object}, "{class_node.spelling}"{superclass_string})'
    )
  else:
    class_output.append(
        f'py::class_<{qualified_name}{superclass_string}{pointer_string}>({parent_object}, "{class_node.spelling}")'
    )
    # Constructors
    if not class_node.is_abstract_record():
      class_output.extend(generate_constructors(class_node, qualified_name))

    # Methods
    class_output.extend(generate_methods(class_node, qualified_name))

    # Fields
    class_output.extend(generate_fields(class_node, qualified_name))

    # Nested enums
    for nested_enum_node in filter(is_not_private, get_nested_enums(class_node)):
      nested_output.extend(
          generate_enum(nested_enum_node, ns_name, qualified_name, submodule_name=submodule_name)
      )

    # Nested types
    for nested_type_node in filter(is_not_private, get_nested_types(class_node)):
      nested_output.extend(
          generate_class(
              nested_type_node,
              ns_name,
              pointer_names,
              qualified_name,
              submodule_name=submodule_name
          )
      )

    if nested_output:
      class_output[
          1
      ] = f'py::class_<{qualified_name}{superclass_string}{pointer_string}> py_{qualified_name.replace("::", "_")}({parent_object}, "{class_node.spelling}");'
      class_output[2] = f'py_{qualified_name.replace("::", "_")}{class_output[2]}'

  class_output.append(';')
  class_binding = Binding(
      qualified_name, True, dependencies=filtered_superclasses, body=class_output
  )
  if parent_class:
    class_binding.dependencies.append(parent_class)

  return [class_binding] + nested_output


def get_namespaces(top_level_node: Cursor) -> List[Cursor]:
  namespaces = []
  if top_level_node.kind == CursorKind.NAMESPACE:  # type: ignore
    namespaces.append(top_level_node)
  namespaces.extend(
      get_nodes_with_kind(top_level_node.get_children(), [CursorKind.NAMESPACE])
  )  # type: ignore
  return namespaces


def get_pointer_defs(top_level_node: Cursor) -> Set[str]:
  typedef_nodes = get_nodes_with_kind(
      top_level_node.get_children(), [CursorKind.TYPEDEF_DECL]
  )  # type: ignore
  return {node.spelling for node in typedef_nodes if node.spelling[-3:].lower() == 'ptr'}


def bind_classes(top_level_node: Cursor) -> List[Binding]:
  output = []
  for ns in get_namespaces(top_level_node):
    # HACK: Only works for singly-nested namespaces
    if ns.semantic_parent.kind == CursorKind.NAMESPACE:  # type: ignore
      qualified_name = f'{ns.semantic_parent.spelling}::{ns.spelling}'
    else:
      qualified_name = ns.spelling

    if ns.spelling in SUBMODULE_NAMESPACES:
      submodule_name = ns.spelling
    else:
      submodule_name = None

    pointer_names = get_pointer_defs(ns)
    for class_node in get_nodes_with_kind(
        ns.get_children(),
        [
            CursorKind.CLASS_DECL,  # type: ignore
            CursorKind.STRUCT_DECL  # type: ignore
        ]):
      output.extend(
          generate_class(class_node, qualified_name, pointer_names, submodule_name=submodule_name)
      )
  return output


# TODO: Templates
def bind_functions(top_level_node: Cursor) -> List[Binding]:
  function_bindings = []
  for ns in get_namespaces(top_level_node):
    # HACK: Only works for singly-nested namespaces
    if ns.semantic_parent.kind == CursorKind.NAMESPACE:  # type: ignore
      qualified_name = f'{ns.semantic_parent.spelling}::{ns.spelling}'
    else:
      qualified_name = ns.spelling
    ns_functions: Dict[str, List[Cursor]] = defaultdict(list)
    # We do this in two stages to handle overloads
    for function_node in get_nodes_with_kind(ns.get_children(),
                                             [CursorKind.FUNCTION_DECL]):  # type: ignore
      ns_functions[to_snake_case(function_node.spelling)].append(function_node)

    parent_object = qualified_name if ns.spelling in SUBMODULE_NAMESPACES else "m"
    for function_name, function_nodes in ns_functions.items():
      if function_name[:8] == 'operator':
        continue

      if len(function_nodes) > 1:
        function_body = [parent_object]
        function_body.extend(generate_overloads(function_name, function_nodes, qualified_name))
        function_body.append(';')
        function_bindings.append(
            Binding(name=function_name, is_class=False, dependencies=[], body=function_body)
        )
      else:
        function_bindings.append(
            Binding(
                name=function_name,
                is_class=False,
                dependencies=[],
                body=[
                    f'{parent_object}.def("{function_name}", &{qualified_name}::{function_nodes[0].spelling});'
                ]
            )
        )
  return function_bindings


def print_tree(root: Cursor, depth: int = 0):
  print(f'{"".join(["  "] * depth)}{root.displayname}: {root.kind}')
  for child in root.get_children():
    print_tree(child, depth + 1)


def generate_bindings(translation_unit: TranslationUnit) -> List[Binding]:
  bindings = []
  for diagnostic in translation_unit.diagnostics:
    print(diagnostic.format())

  file_nodes = get_nodes_from_file(
      translation_unit.cursor.get_children(), translation_unit.spelling
  )
  for node in file_nodes:
    bindings.extend(bind_classes(node))
    bindings.extend(bind_functions(node))

  return bindings


def toposort_bindings(bindings: List[Binding]) -> List[str]:
  output: List[str] = []
  class_bindings = [binding for binding in bindings if binding.is_class and binding.dependencies]
  frontier = [binding for binding in bindings if binding.is_class and not binding.dependencies]
  while frontier:
    next_binding = frontier.pop()
    output.extend(next_binding.body)
    new_class_bindings = []
    for binding in class_bindings:
      if next_binding.name in binding.dependencies:
        binding.dependencies.remove(next_binding.name)

      if not binding.dependencies:
        frontier.append(binding)
      else:
        new_class_bindings.append(binding)

    class_bindings = new_class_bindings

  for binding in filter(lambda b: not b.is_class, bindings):
    output.extend(binding.body)

  assert not class_bindings, f'Could not resolve all type dependencies! {class_bindings}'
  return output


def process_external(external_imports: Dict[str, bool], b: Binding):
  # Copy because we need to modify the list as we iterate
  i = 0
  while i < len(b.dependencies):
    d = b.dependencies[i]
    for prefix in external_imports:
      if d[:len(prefix)] == prefix:
        external_imports[prefix] = True
        # TODO: Not sure this will use the right name for the external parent type.
        # Need to try to compile and see if the pybind11 magic works
        del b.dependencies[i]
        i -= 1
        break

    i += 1


if __name__ == '__main__':
  arg_parser = argparse.ArgumentParser()
  arg_parser.add_argument(
      '-m', '--module-name', help='The name of the output Python module', type=Path
  )
  arg_parser.add_argument('-o', '--output-file', help='Output file path', type=Path)
  arg_parser.add_argument(
      '-c',
      '--compilation-database',
      help='Directory containing the project compile_commands.json',
      type=Path
  )
  arg_parser.add_argument(
      'headers',
      metavar='HEADER',
      help='Header files to generate bindings for',
      nargs='+',
      type=Path
  )
  args = arg_parser.parse_args()
  prefix = [
      r'''#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <pybind11/stl_bind.h>
#include <pybind11/cast.h>
#include <pybind11/functional.h>''',
      r'''
template <typename ElemT, typename ResultT=ElemT>
std::vector<ResultT> convertVec(const std::vector<ElemT>& vec) {
  return vec;
}

template<>
std::vector<const char*> convertVec(const std::vector<std::string>& vec) {
  std::vector<const char*> result;
  result.reserve(vec.size());
  for(const auto& str: vec) {
    result.push_back(str.c_str());
  }

  return result;
}'''
  ]

  METHOD_REPLACEMENTS['to_pddl'] = (
      '__repr__',
      '[](const {qualified_name} &o){{std::stringstream ss; o.toPDDL(ss); return ss.str();}}'
  )
  METHOD_REPLACEMENTS['to_dot_file'] = (
      '__repr__',
      '[](const {qualified_name} &o){{std::stringstream ss; o.toDotFile(ss); return ss.str();}}'
  )
  METHOD_REPLACEMENTS['to_string'] = (
      '__repr__',
      '[](const {qualified_name} &o){{std::stringstream ss; o.toString(ss); return ss.str();}}'
  )

  body = []

  print('Parsing header files...')
  index, translation_units = load_data(args.compilation_database, args.headers)

  # HACK: We should handle generating submodules for namespaces more generally
  SUBMODULE_NAMESPACES.append('ast')

  print('Generating bindings...')
  bindings = []
  for tu in translation_units:
    bindings.extend(generate_bindings(tu))

  # Handle known external libraries
  # TODO: Parameterize this list as a cmdline arg
  EXTERNAL_MODULES = {'dart': 'dartpy', 'ompl': 'omplpy'}
  external_module_imports = {'dart': False, 'ompl': False}
  for b in bindings:
    process_external(external_module_imports, b)

  # Put in dependency order:
  bodies = toposort_bindings(bindings)

  prefix.extend(f"#include <{header_path.relative_to('include')}>" for header_path in args.headers)
  prefix.extend(['namespace py = pybind11;', f'PYBIND11_MODULE({args.module_name}, m) {{'])
  prefix.extend(
      f'py::module_ {ns_name}_m = m.def_submodule("{ns_name}");' for ns_name in SUBMODULE_NAMESPACES
  )
  prefix.extend(
      f'py::module_::import("{import_name}");' for mod_name,
      import_name in EXTERNAL_MODULES.items() if external_module_imports[mod_name]
  )
  output = prefix + bodies + ['}']
  print(f'Outputting bindings to {args.output_file}')
  with open(args.output_file, 'w') as output_file:
    output_file.writelines([
        line for pair in zip(output, repeat('\n', len(output))) for line in pair
    ])
