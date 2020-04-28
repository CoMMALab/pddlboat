(define (domain logistics-adl)
  (:requirements :adl :domain-axioms)
  (:types physobj - object
          obj vehicle - physobj
          truck airplane - vehicle
          location city - object
          airport - location)
  (:predicates (at ?x - physobj ?l ?y)
               (in ?x - obj ?t - vehicle ?l))
  )
