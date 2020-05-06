(define (problem simple)
    (:domain block-world)
    (:objects
        a - block
        b - block
        c - block

        l1 - location
        l2 - location
        l3 - location

        r1 - robot
        r2 - robot
    )
    (:init
        (at c l1)
        (on b c)
        (on a b)
        (clear a)

        (clear_loc l2)
        (clear_loc l3)

        (reachable l1 r1)
        (reachable l2 r1)

        (reachable l2 r2)
        (reachable l3 r2)

        (handempty r1)
        (handempty r2)
    )
    (:goal
        (and
            (at c l3)
            (on b c)
            (on a b)
        )
    )
)
