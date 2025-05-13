;; Define function tests

;; Define a simple variable
(DEFINE 'pi 314)
pi

;; Define another variable using arithmetic
(DEFINE 'radius 10)
radius

;; Use defined variables in expressions
(MUL pi (MUL radius radius))

;; Define a function (square)
(DEFINE 'square (x) (MUL x x))

;; Use the defined function
(square 5)
(square radius)

;; Define a function with multiple parameters
(DEFINE 'circle-area (r) (MUL pi (square r)))

;; Use the composed function
(circle-area 10)
(circle-area radius)

;; Define a function that uses multiple parameters
(DEFINE 'add3 (a b c) (ADD a (ADD b c)))
(add3 5 10 15)

;; Redefine a variable
(DEFINE 'pi 3141)
pi

(circle-area 10)  ;; Should use the new value of pi

(DEFINE 'recurse (n)
    (COND
      ((EQ n 0) 1)  ;; Base case: 0! = 1
      (T (recurse (SUB n 1)))))
;; recurse 0
(recurse 0)
;; recurse 1
(recurse 1)
;; recurse 2
(recurse 2)

;; More complex function
(DEFINE 'factorial (n)
  (COND
    ((EQ n 0) 1)  ;; Base case: 0! = 1
    (T (MUL n (factorial (SUB n 1))))))

;; Test factorial function
;; fact 0
(factorial 0)  ;; Should be 1
;; fact 1
(factorial 1)  ;; Should be 1

(factorial 2)  ;; Should be 2
(factorial 3)  ;; Should be 6
(factorial 4)  ;; Should be 24
(factorial 5)  ;; Should be 120
