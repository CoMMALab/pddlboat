(define (domain rover-domain)
    (:types float int)
    (:functions
        (battery-amount ?rover) (battery-max ?rover) (battery-min ?rover) - float
        (height ?rover) (width ?rover) (weight ?rover) - int 
    )
  )