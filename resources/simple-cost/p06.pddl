(define (problem map-01)
  (:domain map)
  (:objects a - robot
            l1 - location
            l2 - location
            l3 - location
            l4 - location
            l5 - location
            )
  (:init (at a l1)

         (clear l2)
         (clear l3)
         (clear l4)
         (clear l5)

         (connected l1 l2)
         (= (distance l1 l2) 1)
         (connected l2 l1)
         (= (distance l2 l1) 1)

         (connected l2 l3)
         (= (distance l2 l3) 1)
         (connected l3 l2)
         (= (distance l3 l2) 1)

         (connected l3 l4)
         (= (distance l3 l4) 1)
         (connected l4 l3)
         (= (distance l4 l3) 1)

         (connected l4 l5)
         (= (distance l4 l5) 1)
         (connected l5 l4)
         (= (distance l5 l4) 1)

         (connected l1 l5)
         (= (distance l1 l5) 10)
         (connected l5 l1)
         (= (distance l5 l1) 10)

         (= (total-cost) 0)
         )

  (:goal (and (at a l5)
              )
         )
  )