(define (problem simple)
    (:domain block-world)
    (:objects
        a - block
        b - block
        c - block
        d - block
        e - block
        f - block
        x - location
        y - location
    )
    (:init 
        (at c x)
        (on b c)
        (on a b)
        (at f y)
        (on e f)
        (on d e)
        (clear d)
        (clear a)
    )
    (:goal 
        (and 
            (at a y)
            (on b a)
            (on c b)
            (at d x)
            (on e d)
            (on f e)
        )
    )
)