;;; Should fail, no :typing requirement specified.
(define (domain test-domain)
  (:types
    physobj - object
    obj vehicle - phssobj
    )
  )