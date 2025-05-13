;; Simple recursion test

;; Recursive identity function
(DEFINE 'recurse-id (n)
  (COND
    ((EQ n 0) 0)
    (T (recurse-id (SUB n 1)))))

;; Test base case
(recurse-id 0)

;; Test simple recursion
(recurse-id 1)
(recurse-id 2)

;; Now test a function that does something more than identity
(DEFINE 'sum (n)
  (COND
    ((EQ n 0) 0)
    (T (ADD n (sum (SUB n 1))))))

(sum 0)
(sum 1)
(sum 2)
(sum 3)
(sum 5)