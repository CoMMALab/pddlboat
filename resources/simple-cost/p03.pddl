(define (problem map-01)
  ; square with diagonals

  (:domain map)
  (:objects a - robot
            l11 l12 l13 - location
            l21 l22 l23 - location
            l31 l32 l33 - location
            )
  (:init
         (clear l12) (clear l13)
         (clear l21) (clear l22) (clear l23)
         (clear l31) (clear l32) (clear l33)

         (at a l11)

         (connected l11 l12) (connected l12 l11) (= (distance l11 l12) 10) (= (distance l12 l11) 10)
         (connected l13 l12) (connected l12 l13) (= (distance l13 l12) 10) (= (distance l12 l13) 10)
         (connected l21 l22) (connected l22 l21) (= (distance l21 l22) 10) (= (distance l22 l21) 10)
         (connected l23 l22) (connected l22 l23) (= (distance l23 l22) 10) (= (distance l22 l23) 10)
         (connected l31 l32) (connected l32 l31) (= (distance l31 l32) 10) (= (distance l32 l31) 10)
         (connected l33 l32) (connected l32 l33) (= (distance l33 l32) 10) (= (distance l32 l33) 10)

         (connected l11 l21) (connected l11 l21) (= (distance l11 l21) 10) (= (distance l21 l11) 10)
         (connected l31 l21) (connected l31 l21) (= (distance l31 l21) 10) (= (distance l21 l31) 10)
         (connected l12 l22) (connected l12 l22) (= (distance l12 l22) 10) (= (distance l22 l12) 10)
         (connected l32 l22) (connected l32 l22) (= (distance l32 l22) 10) (= (distance l22 l32) 10)
         (connected l13 l23) (connected l13 l23) (= (distance l13 l23) 10) (= (distance l23 l13) 10)
         (connected l33 l23) (connected l33 l23) (= (distance l33 l23) 10) (= (distance l23 l33) 10)

         (connected l11 l22) (connected l22 l11) (= (distance l11 l22) 1) (= (distance l22 l11) 1)
         (connected l33 l22) (connected l22 l33) (= (distance l33 l22) 1) (= (distance l22 l33) 1)

         (connected l12 l23) (connected l23 l12) (= (distance l12 l23) 1) (= (distance l23 l12) 1)
         (connected l21 l32) (connected l32 l21) (= (distance l21 l32) 1) (= (distance l32 l21) 1)

         (connected l13 l22) (connected l22 l13) (= (distance l13 l22) 1) (= (distance l22 l13) 1)
         (connected l31 l22) (connected l22 l31) (= (distance l31 l22) 1) (= (distance l22 l31) 1)

         (connected l12 l21) (connected l21 l12) (= (distance l12 l21) 1) (= (distance l21 l12) 1)
         (connected l23 l32) (connected l32 l23) (= (distance l23 l32) 1) (= (distance l32 l23) 1)

         (= (total-cost) 0)
         )

  (:goal (and (at a l31)
              )
         )
  )
