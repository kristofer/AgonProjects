;; Turn on debugging
(DEBUG 1)

;; Simple factorial function
(DEFINE 'factorial (n)
  (COND
    ((EQ n 0) 1)
    (T (MUL n (factorial (SUB n 1))))))

;; Base case
(factorial 0)

;; Simple case
(factorial 1)

;; More complex cases
(factorial 3)
(factorial 5)

(DEBUG 0)