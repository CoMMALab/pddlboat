; It contains 2 towers, 3 blocks each, to build 2 towers in the same order
(define (problem tmp)
    (:domain reachability-world)
    (:objects
        thing1 - robot
        ta - region
        tb - region
        bu - block
        xaxis - orientation
        yaxis - orientation
    )
    (:init 
        (at bu ta)
        (clear bu)
        (handempty thing1)
        (reach-at bu ta)
        (reachable ta thing1)
        (reachable tb thing1)
    )
    (:goal 
        (and
            (at bu tb)
        )
    )
)
