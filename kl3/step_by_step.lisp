;; Turn on debug mode
(DEBUG 1)

;; Non-recursive function
(DEFINE 'test-func (n) (EQ n 0))

;; Test the function
(test-func 0)
(test-func 1)

;; Function that returns another value when (EQ n 0) is false
(DEFINE 'test-func2 (n)
  (COND
    ((EQ n 0) 0)
    (T 100)))

;; Test the function
(test-func2 0)
(test-func2 1)

;; Function that uses a param twice
(DEFINE 'double (n) (ADD n n))
(double 5)

;; Function that uses another function
(DEFINE 'test-func3 (n)
  (double n))

(test-func3 3)

;; Function that calls itself with a hardcoded value 0
;; This tests if the function can find itself during recursion
(DEFINE 'call-myself (n)
  (COND
    ((EQ n 0) 0)
    (T (call-myself 0))))

;; This should return 0 since we call with 0 directly
(call-myself 0)

;; This should call (call-myself 0) which returns 0
(call-myself 1)

;; Turn off debug mode
(DEBUG 0)