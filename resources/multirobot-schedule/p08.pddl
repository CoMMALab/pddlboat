(define (problem simple)
  (:domain block-world)
  (:objects
   a - block
   b - block
   c - block
   d - block

   l1 - location
   l2 - location
   l3 - location
   l4 - location

   r1 - robot
   r2 - robot
   r3 - robot
   r4 - robot
   )
  (:init
   (at a l1)
   (clear a)

   (at b l2)
   (clear b)

   (at c l3)
   (clear c)

   (at d l4)
   (clear d)

   (reachable l1 r1)
   (reachable l2 r2)
   (reachable l3 r3)
   (reachable l4 r4)

   (handempty r1)
   (handempty r2)
   (handempty r3)
   (handempty r4)

   (can-handoff r1 r2)
   (can-handoff r2 r3)
   (can-handoff r3 r4)
   )
  (:goal
   (and
    (at a l2)
    (at b l3)
    (at c l4)
    (at d l1)
    )
   )
  )
