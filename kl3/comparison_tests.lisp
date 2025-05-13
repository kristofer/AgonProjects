;; Comparison function tests

;; Turn on debugging
(DEBUG 1)

;; Test LT (Less Than)
(LT 5 10)  ;; Should be T (5 < 10)
(LT 10 5)  ;; Should be NIL (10 < 5)
(LT 5 5)   ;; Should be NIL (5 < 5)

;; Test LTE (Less Than or Equal)
(LTE 5 10) ;; Should be T (5 <= 10)
(LTE 10 5) ;; Should be NIL (10 <= 5)
(LTE 5 5)  ;; Should be T (5 <= 5)

;; Test GT (Greater Than)
(GT 5 10)  ;; Should be NIL (5 > 10)
(GT 10 5)  ;; Should be T (10 > 5)
(GT 5 5)   ;; Should be NIL (5 > 5)

;; Test GTE (Greater Than or Equal)
(GTE 5 10) ;; Should be NIL (5 >= 10)
(GTE 10 5) ;; Should be T (10 >= 5)
(GTE 5 5)  ;; Should be T (5 >= 5)

;; Test with variables
(DEFINE 'a 10)
(DEFINE 'b 20)

(LT a b)   ;; Should be T (10 < 20)
(GT a b)   ;; Should be NIL (10 > 20)

;; Test in a function
(DEFINE 'min-func (a b)
  (COND
    ((LT a b) a)
    (T b)))

(min-func 5 10)  ;; Should return 5
(min-func 10 5)  ;; Should return 5

;; Test in a function
(DEFINE 'max-func (a b)
  (COND
    ((GT a b) a)
    (T b)))

(max-func 5 10)  ;; Should return 10
(max-func 10 5)  ;; Should return 10

;; Turn off debugging
(DEBUG 0)