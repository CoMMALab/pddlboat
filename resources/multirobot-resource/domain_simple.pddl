(define (domain block-world)
  (:requirements :strips)
  (:types block location robot)
  (:predicates (at ?x - block ?y - location)
               (clear_loc ?x - location)
               (in_hand ?x - block ?y - robot))

  (:derived (handempty ?x - robot)
            (forall (?y - block)
                    (not (in_hand ?y ?x))))

  (:action pick-up
           :parameters (?x - block ?y - location ?z - robot)
           :precondition (and (at ?x ?y)
                              (handempty ?z))
           :effect (and (not (at ?x ?y))
                        (in_hand ?x ?z)
                        (clear_loc ?y))
           )

  (:action put-down
           :parameters (?x - block ?y - location ?z - robot)
           :precondition (and (clear_loc ?y)
                              (in_hand ?x ?z))
           :effect (and (at ?x ?y)
                        (not (in_hand ?x ?z))
                        (not (clear_loc ?y)))
           )
  )
