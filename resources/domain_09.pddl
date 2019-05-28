;;; Taken from TMKit
(define (domain blocksworld)
  (:requirements :typing)
  (:types abstract-location - object
          location - abstract-location
          fixed-location - abstract-location
          abstract-object - abstract-location
          block - abstract-object)
  (:constants no-object - abstract-object)
  )