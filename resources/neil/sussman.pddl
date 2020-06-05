(define (problem sussman)
  (:domain blocksworld)
  (:objects block-a block-b block-c block-x - block
            loc-0 loc-1 loc-2 - location
            )
  (:init (= (position block-c) block-a)
         (= (position block-b) loc-0)
         (= (position block-a) loc-2)
         (= (position block-x) loc-1)
         )
  (:goal (and (= (position block-c) loc-1)
              (= (position block-a) block-b)
              (= (position block-b) block-c))))
