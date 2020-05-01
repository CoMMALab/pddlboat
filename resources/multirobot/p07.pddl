(define (problem simple)
    (:domain block-world)
    (:objects
        a - block
        b - block
        c - block
        x - location
        y - location
    )
    (:init 
        (at c x)
        (on b c)
        (on a b)
        (clear a)
        (clear_loc y)
    )
    (:goal 
        (and 
            (at c y)
            (on b c)
            (on a b)
        )
    )
)