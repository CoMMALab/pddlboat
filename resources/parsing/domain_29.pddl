(define (domain trashdom)
  (:requirements :typing)
  (:types abstract-location - object
          location - abstract-location
          fixed-location - abstract-location
          tray-location - abstract-location
          abstract-object - abstract-location
          tray - abstract-object
          block - abstract-object)
  (:constants no-object - abstract-object
              trash - abstract-location
              )
  (:predicates (finalized ?obj - block)
               (gone ?obj - block)
               (pushed))


  (:functions (position ?obj - abstract-object) - abstract-location)

  (:derived (occupied ?loc - abstract-location)
            (exists (?obj - block)
                    (= (position ?obj) ?loc)))

  (:derived (moveable ?obj - block)
            (and (not (finalized ?obj))
                 (not (occupied ?obj))))

  (:action transfer ; move object to other location
           :parameters (?obj - block ?loc-1 - location)
           :precondition (and (moveable ?obj)
                              (not (pushed))
                              (not (occupied ?loc-1)))
           :effect (and (= (position ?obj)
                           ?loc-1)))
  (:action stack ; stack one object on another
           :parameters (?obj - block
                             ?loc-1 - block)
           :precondition (and (moveable ?obj)
                              (not (pushed))
                              (not (gone ?loc-1))
                              (not (occupied ?loc-1)))
           :effect (and (= (position ?obj) ?loc-1)))
  (:action final-transfer ; move object to final location
           :parameters (?obj - block ?loc-1 - fixed-location)
           :precondition (and (moveable ?obj)
                              (not (pushed))
                              (not (occupied ?loc-1)))
           :effect (and (= (position ?obj)
                           ?loc-1)
                        (finalized ?obj)))
  (:action push-tray ; move tray
           :parameters (?obj - tray ?loc-1 - tray-location)
           :precondition (and (not (= (position ?obj) ?loc-1)))
           :effect (and (= (position ?obj) ?loc-1)
                        (pushed)))
  )
