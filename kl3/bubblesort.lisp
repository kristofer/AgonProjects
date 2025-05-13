;; Bubble Sort implementation for kl3

;; Turn on debugging to see what's happening
(DEBUG 1)

;; First, a simple sort2 function that can sort 2 elements
(DEFINE 'sort2 (a b)
  (COND
    ((LT a b) (CONS a (CONS b NIL)))
    (T (CONS b (CONS a NIL)))))

;; Test sort2
(sort2 5 3)  ;; Should return (3 5)
(sort2 2 7)  ;; Should return (2 7)

;; Helper to check if two adjacent elements are in the right order
;; and swap them if needed
(DEFINE 'check-and-swap (a b)
  (COND
    ((LT a b) (CONS a (CONS b NIL)))  ;; Already in order
    (T (CONS b (CONS a NIL)))))       ;; Need to swap

;; Perform one bubble sort pass - one element at a time
;; For each adjacent pair, we check and swap if needed
(DEFINE 'bubble-pass (lst)
  (COND
    ((ATOM lst) NIL)                  ;; Empty list
    ((ATOM (CDR lst)) (CONS (CAR lst) NIL)) ;; Single element list
    (T (CONS (CAR (check-and-swap (CAR lst) (CAR (CDR lst))))
             (bubble-pass 
               (CONS (CAR (CDR (check-and-swap (CAR lst) (CAR (CDR lst)))))
                     (CDR (CDR lst))))))))

;; Helper function to determine if a list is sorted
(DEFINE 'is-sorted (lst)
  (COND
    ((ATOM lst) T)                   ;; Empty list is sorted
    ((ATOM (CDR lst)) T)             ;; Single element list is sorted
    ((GT (CAR lst) (CAR (CDR lst))) NIL) ;; Out of order
    (T (is-sorted (CDR lst)))))      ;; Check rest of list

;; A simpler bubble sort implementation that does one full pass
;; and then recursively calls itself if the list isn't sorted
(DEFINE 'bubblesort (lst)
  (COND
    ((ATOM lst) NIL)                  ;; Empty list
    ((ATOM (CDR lst)) lst)            ;; Single element list
    ((is-sorted lst) lst)             ;; Already sorted
    (T (bubblesort (bubble-pass lst)))))

;; Let's turn off verbose debugging
(DEBUG 0)

;; Test with single elements
(bubblesort '())     ;; Should be NIL (empty list)
(bubblesort '(3))    ;; Should be (3) (single element)

;; Test with pre-sorted list
(bubblesort '(1 2 3 4 5))  ;; Should be (1 2 3 4 5)

;; Test with reverse-sorted list
(bubblesort '(5 4 3 2 1))  ;; Should be (1 2 3 4 5)

;; Test with mixed list
(bubblesort '(5 7 3 2 0 9))  ;; Should be (0 2 3 5 7 9)

;; Done!