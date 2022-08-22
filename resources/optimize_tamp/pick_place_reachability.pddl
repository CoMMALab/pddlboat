(define (domain reachability-world)
  (:requirements :typing)

  (:types block - object
          robot - object
          location - object
          region - location
          limited-location - location)

  (:predicates (at ?b - block ?l - location)        ; Is ?b at ?l
               (reach-at ?b - block ?l - location)  ; Is ?b reachable by being at or on ?l 
               (in-hand ?b - block ?r - robot)      ; Is ?b grasped by ?r
               (handempty ?r - robot)               ; Is ?r not grasping anything
               (reachable ?l - location ?r - robot) ; Can ?r reach ?l
               (clear_loc ?l - limited-location)
               )

  (:action put-down
           :parameters (?b - block ?l - region ?r - robot)
           :precondition (and (in-hand ?b ?r)
                              (reachable ?l ?r))
           :effect (and (at ?b ?l)
                        (reach-at ?b ?l)
                        (not (in-hand ?b ?r))
                        (handempty ?r))
           )

  (:action pick-up-limited
           :parameters (?b - block ?l - limited-location ?r - robot)
           :precondition (and (at ?b ?l)
                              (handempty ?r)
                              (reachable ?l ?r)
                              (reach-at ?b ?l))
           :effect (and (not (at ?b ?l))
                        (not (reach-at ?b ?l))
                        (in-hand ?b ?r)
                        (not (handempty ?r))
                        (clear_loc ?l))
           )

  (:action put-down-limited
           :parameters (?b - block ?l - limited-location ?r - robot)
           :precondition (and (in-hand ?b ?r)
                              (reachable ?l ?r)
                              (clear_loc ?l))
           :effect (and (at ?b ?l)
                        (reach-at ?b ?l)
                        (not (in-hand ?b ?r))
                        (handempty ?r)
                        (not (clear_loc ?l)))
           )
  )
