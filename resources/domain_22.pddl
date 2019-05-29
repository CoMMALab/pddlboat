(define (domain elevators-sequencedstrips)
  (:requirements :action-costs :typing)
  (:types elevator - object
  	      slow-elevator - elevator
  	      fast-elevator - elevator
  	      passenger - object
  	      count - object)
  (:predicates (passenger-at ?passenger0 - passenger ?count1 - count)
	             (boarded ?passenger0 - passenger ?elevator1 - elevator)
	             (lift-at ?elevator0 - elevator ?count1 - count)
	             (reachable-floor ?elevator0 - elevator ?count1 - count)
	             (above ?count0 - count ?count1 - count)
	             (passengers ?elevator0 - elevator ?count1 - count)
	             (can-hold ?elevator0 - elevator ?count1 - count)
	             (next ?count0 - count ?count1 - count))
  (:functions (total-cost)
  	          (travel-slow ?count0 - count ?count1 - count)
  	          (travel-fast ?count0 - count ?count1 - count))
  )
