;; Testing non-recursive function first

(DEFINE 'simple-func (n)
  (COND
    ((EQ n 0) 0)
    (T 100)))

;; Try the base case
(simple-func 0)

;; Try a non-base case
(simple-func 1)

;; Double a number (non-recursive)
(DEFINE 'double (n)
  (ADD n n))

(double 5)
(double 10)

;; Display env to see current bindings
(DEBUG 1)
(DEBUG 0)