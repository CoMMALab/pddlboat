(define (problem simple)
    (:domain block-world)
    (:objects
        a - block
        b - block
        c - block
        d - block
        e - block
        f - block
        g - block
        h - block
        i - block
        j - block
        k - block
        l - block
        w - location
        x - location
        y - location
        z - location
    )
    (:init 
        (at c w)
        (on b c)
        (on a b)

        (at f x)
        (on e f)
        (on d e)

        (at i y)
        (on h i)
        (on g h)

        (at l z)
        (on k l)
        (on j k)

        (clear a)
        (clear d)
        (clear g)
        (clear j)
    )
    (:goal 
        (and 
            (at a x)
            (on b a)
            (on c b)

            (at d y)
            (on e d)
            (on f e)

            (at g z)
            (on h g)
            (on i h)

            (at j w)
            (on k j)
            (on l k)
        )
    )
)