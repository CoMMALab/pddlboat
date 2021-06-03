(define (problem map-01)
  (:domain map)
  (:objects a - robot
            b - robot
            l1 - location
            l2 - location
            l3 - location
            l4 - location
            l5 - location
            l6 - location
            l7 - location
            )
  (:init (at a l1)
         (at b l5)

         (clear l2)
         (clear l3)
         (clear l4)

         (clear l6)
         (clear l7)

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

         (connected l6 l5)
         (= (distance l6 l5) 1)
         (connected l5 l6)
         (= (distance l5 l6) 1)

         (connected l7 l5)
         (= (distance l7 l5) 1)
         (connected l5 l7)
         (= (distance l5 l7) 1)

         (= (total-cost) 0)
         )

  (:goal (and (at a l5) (at b l1))
         )
  )