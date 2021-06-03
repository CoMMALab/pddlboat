(define (problem sussman)
  (:domain blocksworld)
  (:objects block-a block-b - block
            loc-0 loc-1 - location
            )
  (:init (= (position block-b) block-a)
         (= (position block-a) loc-0)
         )
  (:goal (and (= (position block-a) block-b)
              (= (position block-b) loc-1))))
