(define (problem simple)
    (:domain block-world)
    (:objects
        a - block
        b - block

        l1 - location
        l2 - location
        l3 - location

        r1 - robot
        r2 - robot
        r3 - robot
        r4 - robot
        r5 - robot
    )
    (:init
        (at a l1)
        (at b l2)
        (clear a)
        (clear b)

        (clear_loc l3)

        (reachable l1 r1)
        (reachable l2 r5)

        (reachable l3 r3)

        (handempty r1)
        (handempty r2)
        (handempty r3)
        (handempty r4)
        (handempty r5)

        (can-handoff r1 r2)
        (can-handoff r2 r3)
        (can-handoff r3 r4)
        (can-handoff r4 r5)
    )
    (:goal
        (and
            (at b l1)
            (at a l2)
        )
    )
)
