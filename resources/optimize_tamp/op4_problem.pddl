(define (problem tmp)
    (:domain reachability-world)
    (:objects
        ta - region
        thing1 - robot
        ta-eb - limited-location
        ta-cb - limited-location
        bx - block
        ta-bb - limited-location
        by - block
        ta-ab - limited-location
        bz - block
        ta-bc - limited-location
        bw - block
    )
    (:init 
        (at bw ta-bc)
        (at bx ta-cb)
        (at by ta-bb)
        (at bz ta-ab)
        (clear_loc ta-eb)
        (handempty thing1)
        (reach-at bw ta-bc)
        (reach-at bx ta-cb)
        (reach-at by ta-bb)
        (reach-at bz ta-ab)
        (reachable ta thing1)
        (reachable ta-ab thing1)
        (reachable ta-bb thing1)
        (reachable ta-bc thing1)
        (reachable ta-cb thing1)
        (reachable ta-eb thing1)
    )
    (:goal 
        (and
            (at by ta-eb)
            (reach-at by ta-eb))
    )
)
