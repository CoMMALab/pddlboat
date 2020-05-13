(define (problem simple)
    (:domain block-world)
    (:objects
        x - block
        a - location
        b - location
        r1 - robot
    )
    (:init 
        (at x a)
        (clear_loc b)
    )
    (:goal 
        (and 
            (at x b)
        )
    )
)