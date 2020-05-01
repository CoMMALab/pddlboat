(define (problem simple)
    (:domain block-world)
    (:objects
        a - block
        b - block
        c - block
        d - block
        x - location
        y - location
        z - location
    )
    (:init 
        (at a x)
        (at b y)
        (at c z)
        (on d a)
        (clear b)
        (clear c)
        (clear d)
    )
    (:goal 
        (and 
            (at a y)
            (at b z)
            (at c x)
            (on d a)
        )
    )
)