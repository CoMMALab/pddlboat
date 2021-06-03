(define (domain map)
  (:requirements :strips)
  (:types location robot)
  (:predicates (at ?r - robot ?l - location)
               (clear ?l - location)
               (connected ?x - location ?y - location)
               )

  (:functions (total-cost) - number
              (distance ?f1 - location ?f2 - location) - number
              )

  (:action go-to
           :parameters (?x - robot ?to - location ?from - location)
           :precondition (and (clear ?to)
                              (at ?x ?from)
                              (connected ?to ?from))
           :effect (and (not (clear ?to))
                        (not (at ?x ?from))
                        (clear ?from)
                        (at ?x ?to)
                        (increase (total-cost) (distance ?from ?to))
                        )
           )
  )
