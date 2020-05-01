(define (problem simple)
    (:domain block-world)
    (:objects
        a - block
        b - block
        c - block
        d - block
        w - location
        x - location
        y - location
        z - location
    )
    (:init 
        (at a w)
        (at b x)
        (at c y)
        (at d z)
        (clear a)
        (clear b)
        (clear c)
        (clear d)
    )
    (:goal 
        (and 
            (at a x)
            (at b y)
            (at c z)
            (at d w)
        )
    )
)