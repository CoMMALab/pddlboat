(define (problem simple)
    (:domain block-world)
    (:objects
        x - block
        y - block
        z - block
        a - location
        b - location
        c - location
        w - location
        r1 - robot
    )
    (:init 
        (at x a)
        (at y b)
        (at z c)
        (clear x)
        (clear y)
        (clear z)
        (clear_loc w)
    )
    (:goal 
        (and 
            (at z w)
            (on y z)
            (on x y)
        )
    )
)