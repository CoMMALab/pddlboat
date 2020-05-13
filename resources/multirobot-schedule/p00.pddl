(define (problem simple)
    (:domain block-world)
    (:objects
        a - block
        b - block

        l1 - location
        l2 - location

        r1 - robot
        r2 - robot
    )
    (:init
        (at b l1)
        (clear b)

        (at a l2)
        (clear a)

        (reachable l1 r1)
        (reachable l2 r1)

        (reachable l1 r2)
        (reachable l2 r2)

        (handempty r1)
        (handempty r2)
    )
    (:goal
        (and
         (at a l1)
         (on b a)
        )
    )
)
