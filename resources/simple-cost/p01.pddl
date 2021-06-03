(define (problem map-01)
  (:domain map)
  (:objects a - robot
            l1 l2 - location
            )
  (:init (clear l2)
         (at a l1)

         (connected l1 l2)
         (connected l2 l1)
         (= (distance l1 l2) 1)
         (= (distance l2 l1) 1)

         (= (total-cost) 0)
         )

  (:goal (and (at a l2)
              )
         )
  )
