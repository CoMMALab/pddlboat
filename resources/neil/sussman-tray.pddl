(define (problem sussman)
  (:domain trashdom)
  (:objects block-a block-b block-c block-x - block
            tray-a - tray
            loc-0 loc-1 loc-2 - location
            tloc-0 tloc-1 - tray-location
            )
  (:init (= (position block-c) block-a)
         (= (position block-b) loc-0)
         (= (position block-a) loc-2)
         (= (position block-x) loc-1)
         (= (position tray-a) tloc-0)
         )
  (:goal (and (= (position tray-a)  tloc-1)
              (= (position block-c) loc-1)
              (= (position block-a) block-b)
              (= (position block-b) block-c))
         )
  )
