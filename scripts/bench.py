import glob
import subprocess
import pathlib
import importlib


def run_planning(domain, problem):
    result = subprocess.run([
        './build/pddlboat_planning',
        '-d',
        domain,
        '-p',
        problem,
        '-s',
        'fd',
        '-y',
        'greedy',
        '--sas_reduce',
        # '-s',
        # 'ff',
        '--oa',
    ],
                            stdout=subprocess.PIPE)
    output = result.stdout.decode('utf-8')

    return output


def run_skills(domain, problem):
    result = subprocess.run([
        './build/pddlboat_skills',
        '-d',
        domain,
        '-p',
        problem,
        '-s',
        'ff',
        # '-y',
        # 'greedy',
        '--seed',
        '1',
        '--sampled_problems',
        '20',
    ],
                            stdout=subprocess.PIPE)
    output = result.stdout.decode('utf-8')

    return result.returncode == 0, output


path = pathlib.Path('resources/icaps/classical/')
domains = {}
for problem in glob.glob(str(path / '*')):
    problem_path = pathlib.Path(problem)
    problem_module = problem_path / "api.py"
    problem_name = problem_path.parts[-1]

    spec = importlib.util.spec_from_file_location(problem_name, problem_module)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)

    domain_info = getattr(module, 'domains')
    domains[problem_name] = domain_info

todo = [domains['blocks']]
# for domain_list in domains.values():
for domain_list in todo:
    for domain in domain_list:
        # if not (domain['ipc'] == '2018' and 'sat' in domain['description']):
        # if not (domain['ipc'] == '2000'):
        #     continue

        print(domain['name'])

        for problem in sorted(domain['problems']):
            domain_file = path / problem[0]
            problem_file = path / problem[1]

            problem_name = problem_file.parts[-1].split('.')[0]

            print(domain_file, problem_file)

            success, output = run_skills(domain_file, problem_file)
            # output = run_planning(domain_file, problem_file)

            if success:
                with open(f"output/{domain['name']}_{problem_name}.txt", 'w') as f:
                    f.write(output)
