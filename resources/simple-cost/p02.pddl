(define (problem map-01)
  ; square with diagonals

  (:domain map)
  (:objects a - robot
            l11 l12 - location
            l21 l22 - location
            )
  (:init
         (clear l12)
         (clear l21)
         (clear l22)

         (at a l11)

         (connected l11 l12) (connected l12 l11)
         (= (distance l11 l12) 1) (= (distance l12 l11) 1)

         (connected l11 l21) (connected l21 l11)
         (= (distance l11 l21) 1) (= (distance l21 l11) 1)

         (connected l22 l12) (connected l12 l22)
         (= (distance l22 l12) 1) (= (distance l12 l22) 1)

         (connected l21 l22) (connected l22 l21)
         (= (distance l21 l22) 1) (= (distance l22 l21) 1)

         (connected l11 l22) (connected l22 l11)
         (= (distance l11 l22) 1.2) (= (distance l22 l11) 1.2)

         (connected l12 l21) (connected l21 l12)
         (= (distance l12 l21) 1.2) (= (distance l21 l12) 1.2)

         (= (total-cost) 0)
         )

  (:goal (and (at a l22)
              )
         )
  )