(define (problem simple)
    (:domain block-world)
    (:objects
        a - block
        b - block

        c - block
        d - block

        l1 - location
        l2 - location
        l3 - location

        r1 - robot
        r2 - robot
        r3 - robot
    )
    (:init
        (at b l1)
        (on a b)
        (clear a)

        (at d l3)
        (on c d)
        (clear c)

        (reachable l1 r1)
        (reachable l2 r2)
        (reachable l3 r3)

        (handempty r1)
        (handempty r2)
        (handempty r3)

        (can-handoff r1 r2)
        (can-handoff r2 r3)
    )
    (:goal
        (and
            (at b l3)
            (on a b)

            (at d l1)
            (on c d)
        )
    )
)
