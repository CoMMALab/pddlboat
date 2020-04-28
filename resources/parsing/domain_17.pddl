(define (domain itmp)
  (:requirements :typing)
  (:types cup
          pitcher
          bowl - object
          dripper - cup
          ingredient
          location
          )
  (:predicates (at ?obj - object ?loc - location )
               (occupied ?loc - location)
               (contains ?obj - object ?ingredient)
               (on ?x - object ?y - object)
               (contains-beverage ?cup - cup ?x - ingredient)
               (covered ?x - object)
               (used ?x - object)
               (full ?x - object)
               (empty ?x - object)
               (holding ?x - object)
               (hand-empty)
               )
  )
