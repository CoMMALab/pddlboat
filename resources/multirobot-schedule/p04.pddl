(define (problem simple)
    (:domain block-world)
    (:objects
        a - block
        b - block
        c - block

        d - block
        e - block
        f - block

        l1 - location
        l5 - location

        r1 - robot
        r2 - robot
        r3 - robot
        r4 - robot
        r5 - robot
    )
    (:init
        (at c l1)
        (on b c)
        (on a b)
        (clear a)

        (at f l5)
        (on e f)
        (on d e)
        (clear d)

        (reachable l1 r1)
        (reachable l5 r5)

        (handempty r1)
        (handempty r2)
        (handempty r3)
        (handempty r4)
        (handempty r5)

        (can-handoff r1 r2)
        (can-handoff r1 r3)
        (can-handoff r1 r4)

        (can-handoff r2 r3)
        (can-handoff r3 r4)

        (can-handoff r2 r5)
        (can-handoff r3 r5)
        (can-handoff r4 r5)
    )
    (:goal
        (and
            (at c l5)
            (on b c)
            (on a b)

            (at f l1)
            (on e f)
            (on d e)
        )
    )
)
