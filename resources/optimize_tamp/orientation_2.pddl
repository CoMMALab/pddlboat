(define (domain reachability-world)
  (:requirements :typing)

  (:types block - object
          robot - object
          location - object
          orientation - object
          region - location
          limited-location - location)

  (:predicates (at ?b - block ?l - location)        ; Is ?b at ?l
               (reach-at ?b - block ?l - location)  ; Is ?b reachable by being at or on ?l
               (clear ?b - block)                   ; Can ?b be grasped
               (on ?top - block ?bottom - block)    ; Is ?top on top of ?bottom
               (in-hand ?b - block ?r - robot)      ; Is ?b grasped by ?r
               (handempty ?r - robot)               ; Is ?r not grasping anything
               (reachable ?l - location ?r - robot) ; Can ?r reach ?l
               (can-handoff ?a - robot ?b - robot)  ; Can ?a handoff to ?b
               (clear_loc ?l - limited-location)
               )

  (:action pick-up
           :parameters (?b - block ?l - region ?r - robot ?o - orientation)
           :precondition (and (clear ?b)
                              (at ?b ?l)
                              (handempty ?r)
                              (reachable ?l ?r)
                              (reach-at ?b ?l))
           :effect (and (not (clear ?b))
                        (not (at ?b ?l))
                        (not (reach-at ?b ?l))
                        (in-hand ?b ?r)
                        (not (handempty ?r)))
           )

  (:action put-down
           :parameters (?b - block ?l - region ?r - robot ?o - orientation)
           :precondition (and (in-hand ?b ?r)
                              (reachable ?l ?r))
           :effect (and (clear ?b)
                        (at ?b ?l)
                        (reach-at ?b ?l)
                        (not (in-hand ?b ?r))
                        (handempty ?r))
           )

  ; (:action pick-up-limited
  ;          :parameters (?b - block ?l - limited-location ?r - robot)
  ;          :precondition (and (clear ?b)
  ;                             (at ?b ?l)
  ;                             (handempty ?r)
  ;                             (reachable ?l ?r)
  ;                             (reach-at ?b ?l))
  ;          :effect (and (not (clear ?b))
  ;                       (not (at ?b ?l))
  ;                       (not (reach-at ?b ?l))
  ;                       (in-hand ?b ?r)
  ;                       (not (handempty ?r))
  ;                       (clear_loc ?l))
  ;          )

  ; (:action put-down-limited
  ;          :parameters (?b - block ?l - limited-location ?r - robot)
  ;          :precondition (and (in-hand ?b ?r)
  ;                             (reachable ?l ?r)
  ;                             (clear_loc ?l))
  ;          :effect (and (clear ?b)
  ;                       (at ?b ?l)
  ;                       (reach-at ?b ?l)
  ;                       (not (in-hand ?b ?r))
  ;                       (handempty ?r)
  ;                       (not (clear_loc ?l)))
  ;          )

  ; (:action unstack
  ;          :parameters (?top - block ?bottom - block ?l - location ?r - robot)
  ;          :precondition (and (on ?top ?bottom)
  ;                             (clear ?top)
  ;                             (handempty ?r)
  ;                             (reachable ?l ?r)
  ;                             (reach-at ?top ?l)
  ;                             (reach-at ?bottom ?l))
  ;          :effect (and (clear ?bottom)
  ;                       (in-hand ?top ?r)
  ;                       (not (handempty ?r))
  ;                       (not (clear ?top))
  ;                       (not (on ?top ?bottom))
  ;                       (not (reach-at ?top ?l)))
  ;          )

  ; (:action handoff
  ;          :parameters (?b - block ?from - robot ?to - robot)
  ;          :precondition (and (can-handoff ?from ?to)
  ;                             (in-hand ?b ?from)
  ;                             (handempty ?to))
  ;          :effect (and (not (handempty ?to))
  ;                       (handempty ?from)
  ;                       (not (in-hand ?b ?from))
  ;                       (in-hand ?b ?to))
  ;          )

  )
