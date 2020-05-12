(define (problem sussman)
  (:domain trashdom)
  (:objects block-a block-b block-c block-x - block
            tray-a - tray
            loc-0 loc-1 loc-2 - location
            tloc-0 tloc-1 - tray-location
            )
  (:init (= block-a (position block-c))
         (= loc-0 (position block-b))
         (= loc-2 (position block-a))
         (= loc-1 (position block-x))
         (= tloc-0 (position tray-a))
         )
  (:goal (and (= tloc-1 (position tray-a))
              (= loc-1 (position block-c))
              (= block-b (position block-a))
              (= block-c (position block-b)))))
