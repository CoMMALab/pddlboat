(define (problem simple)
    (:domain block-world)
    (:objects
        a - block
        b - block
        c - block
        w - location
        x - location
        y - location
    )
    (:init 
        (at a w)
        (at b x)
        (at c y)
        (clear a)
        (clear b)
        (clear c)
    )
    (:goal 
        (and 
            (at a x)
            (at b y)
            (at c w)
        )
    )
)