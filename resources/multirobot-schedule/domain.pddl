(define (domain block-world)
  (:requirements :strips)

  (:types block
          location
          robot)

  (:predicates (at ?b - block ?l - location)        ; Is ?b at ?l
               (clear ?b - block)                   ; Can ?b be grasped
               (clear_loc ?l - location)            ; Is there no block at ?l
               (on ?top - block ?bottom - block)    ; Is ?top on top of ?bottom
               (in-hand ?b - block ?r - robot)      ; Is ?b grasped by ?r
               (handempty ?r - robot)               ; Is ?r not grasping anything
               (reachable ?l - location ?r - robot) ; Can ?r reach ?l
               (can-handoff ?a - robot ?b - robot)  ; Can ?a handoff to ?b
               )

  (:derived (is-reachable ?b - block ?r - robot) ; Is ?b at a reachable location
            (or
             ;; If stacked on a block, recurse
             (exists (?other - block)
                     (and (on ?b ?other)
                          (is-reachable ?other ?r)))
             ;; If at a location, check reachability
             (exists (?l - location)
                     (and (at ?b ?l)
                          (reachable ?l ?r))))
            )

  (:action pick-up
           :parameters (?b - block ?l - location ?r - robot)
           :precondition (and (clear ?b)
                              (at ?b ?l)
                              (handempty ?r)
                              (reachable ?l ?r))
           :effect (and (not (clear ?b))
                        (not (at ?b ?l))
                        (in-hand ?b ?r)
                        (not (handempty ?r))
                        (clear_loc ?l))
           )

  (:action put-down
           :parameters (?b - block ?l - location ?r - robot)
           :precondition (and (clear_loc ?l)
                              (in-hand ?b ?r)
                              (reachable ?l ?r))
           :effect (and (clear ?b)
                        (at ?b ?l)
                        (not (in-hand ?b ?r))
                        (handempty ?r)
                        (not (clear_loc ?l)))
           )

  (:action stack
           :parameters (?top - block ?bottom - block ?r - robot)
           :precondition (and (clear ?bottom)
                              (in-hand ?top ?r)
                              (is-reachable ?bottom ?r))
           :effect (and (not (clear ?bottom))
                        (not (in-hand ?top ?r))
                        (handempty ?r)
                        (clear ?top)
                        (on ?top ?bottom))
           )

  (:action unstack
           :parameters (?top - block ?bottom - block ?r - robot)
           :precondition (and (on ?top ?bottom)
                              (clear ?top)
                              (handempty ?r)
                              (is-reachable ?bottom ?r))
           :effect (and (clear ?bottom)
                        (in-hand ?top ?r)
                        (not (handempty ?r))
                        (not (clear ?top))
                        (not (on ?top ?bottom)))
           )

  (:action handoff
           :parameters (?b - block ?from - robot ?to - robot)
           :precondition (and (or (exists (?l - location)
                                      (and (reachable ?l ?from)
                                           (reachable ?l ?to)))
                                  (can-handoff ?from ?to)
                                  (can-handoff ?to ?from))
                              (in-hand ?b ?from)
                              (handempty ?to))
           :effect (and (not (handempty ?to))
                        (handempty ?from)
                        (not (in-hand ?b ?from))
                        (in-hand ?b ?to))
           )
  )

