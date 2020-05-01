(define (problem simple)
    (:domain block-world)
    (:objects
        a - block
        b - block
        x - location
        y - location
        z - location
    )
    (:init 
        (at a x)
        (at b y)
        (clear a)
        (clear b)
        (clear_loc z)
    )
    (:goal 
        (and 
            (at a y)
            (at b x)
        )
    )
)