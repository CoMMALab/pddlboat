import sys
sys.path.append('.')

import pddlboat as pd

domain = pd.Domain("block-world");
domain.add_type("block", "object");

p_on = pd.make_pred("on", [pd.Variable("x", "block"), pd.Variable("y", "block")]);
domain.addPredicate(p_on);

p_ontable = pd.make_pred("ontable", {{"x", "block"}});
domain.addPredicate(p_ontable);

p_clear = pd.make_pred("clear", {{"x", "block"}});
domain.addPredicate(p_clear);

p_empty = pd.make_pred("handempty");
domain.addPredicate(p_empty);

p_holding = pd.make_pred("holding", {{"x", "block"}});
domain.addPredicate(p_holding);

    # auto a_pickup =  //
    #     pd::makeAction("pick-up",
    #                    // Parameters
    #                    {{"x", "block"}},  //
    #                    // Precondition
    #                    pd::makeAnd({p_clear->bind({"?x"}), p_ontable->bind({"?x"}), p_empty->bind()}),
    #                    // Effect
    #                    pd::makeAnd({pd::makeNot(p_clear->bind({"?x"})),    //
    #                                 pd::makeNot(p_ontable->bind({"?x"})),  //
    #                                 pd::makeNot(p_empty->bind()),          //
    #                                 p_holding->bind({"?x"})}));
    # domain->addAction(a_pickup);

    # auto a_putdown =  //
    #     pd::makeAction("put-down",
    #                    // Parameters
    #                    {{"x", "block"}},  //
    #                    // Precondition
    #                    pd::makeAnd({p_holding->bind({"?x"})}),
    #                    // Effect
    #                    pd::makeAnd({p_clear->bind({"?x"}),    //
    #                                 p_ontable->bind({"?x"}),  //
    #                                 p_empty->bind(),          //
    #                                 pd::makeNot(p_holding->bind({"?x"}))}));
    # domain->addAction(a_putdown);

    # auto a_stack =  //
    #     pd::makeAction("stack",
    #                    // Parameters
    #                    {{"x", "block"}, {"y", "block"}},  //
    #                    // Precondition
    #                    pd::makeAnd({p_holding->bind({"?x"}), p_clear->bind({"?y"})}),
    #                    // Effect
    #                    pd::makeAnd({pd::makeNot(p_holding->bind({"?x"})),  //
    #                                 pd::makeNot(p_clear->bind({"?y"})),    //
    #                                 p_clear->bind({"?x"}),                 //
    #                                 p_empty->bind(),                       //
    #                                 p_on->bind({"?x", "?y"})}));
    # domain->addAction(a_stack);

    # auto a_unstack =  //
    #     pd::makeAction("unstack",
    #                    // Parameters
    #                    {{"x", "block"}, {"y", "block"}},  //
    #                    // Precondition
    #                    pd::makeAnd({p_on->bind({"?x", "?y"}), p_clear->bind({"?x"}), p_empty->bind()}),
    #                    // Effect
    #                    pd::makeAnd({p_holding->bind({"?x"}),             //
    #                                 p_clear->bind({"?y"}),               //
    #                                 pd::makeNot(p_clear->bind({"?x"})),  //
    #                                 pd::makeNot(p_empty->bind()),        //
    #                                 pd::makeNot(p_on->bind({"?x", "?y"}))}));
    # domain->addAction(a_unstack);

    # if (domainout)
    #     domain->toPDDL(std::cout) << std::endl;

    # //
    # // Build Problem
    # //
    # auto problem = std::make_shared<pd::Problem>("simple", domain);
    # problem->addObject("a", "block");
    # problem->addObject("b", "block");
    # problem->addObject("c", "block");
    # problem->addObject("d", "block");

    # problem->start->assign1("clear", "a");
    # problem->start->assign1("clear", "b");
    # problem->start->assign1("clear", "c");
    # problem->start->assign1("clear", "d");
    # problem->start->assign1("ontable", "a");
    # problem->start->assign1("ontable", "b");
    # problem->start->assign1("ontable", "c");
    # problem->start->assign1("ontable", "d");
    # problem->start->assign0("handempty");

    # problem->goal = pd::makeAnd({
    #     p_on->bind({"d", "c"}, problem),
    #     p_on->bind({"c", "b"}, problem),
    #     p_on->bind({"b", "a"}, problem),
    # });

    # if (problemout)
    #     problem->toPDDL(std::cout) << std::endl;

    # return 0;
