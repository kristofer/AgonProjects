(COND (T (QUOTE SUCCESS)))
()
(ATOM 'FOO)
(ATOM ())
(ATOM NIL)
(ATOM (QUOTE BAR))
(ATOM '(A B C))
;; a comment
(CAR '(CAR CDR))
(CAR '(CAR (CDR)))
;; start of bigger tests

;; CONS CELL
;; BUILDING BLOCK OF DATA STRUCTURES
(CONS NIL NIL)
(CONS (QUOTE X) (QUOTE Y))

;; REFLECTION
;; EVERYTHING IS AN ATOM OR NOT AN ATOM
(ATOM NIL)
(ATOM (CONS NIL NIL))

;; QUOTING
;; CODE IS DATA AND DATA IS CODE
(QUOTE (CONS NIL NIL))
(CONS (QUOTE CONS) (CONS NIL (CONS NIL NIL)))

;; LOGIC
;; BY WAY OF STRING INTERNING
(EQ (QUOTE A) (QUOTE A))
(EQ (QUOTE T) (QUOTE F))

;; FIND FIRST ATOM IN TREE
;; CORRECT RESULT OF EXPRESSION IS `A`
;; RECURSIVE CONDITIONAL FUNCTION BINDING
;; this is the program run by the MetaInterpreter below.
((LAMBDA (FF X) (FF X))
 (QUOTE (LAMBDA (X)
          (COND ((ATOM X) X)
                ((QUOTE T) (FF (CAR X))))))
 (QUOTE ((A) B C)))

;; ((EQ (CAR E) (QUOTE LAMBDA)) E)
;; ((EQ (CAR E) ()) (QUOTE *UNDEFINED))
;; LISP IMPLEMENTED IN LISP
;; WITHOUT ANY SUBJECTIVE SYNTACTIC SUGAR
;; RUNS "FIND FIRST ATOM IN TREE" PROGRAM
;; CORRECT RESULT OF EXPRESSION IS STILL `A`
;; REQUIRES CONS CAR CDR QUOTE ATOM EQ LAMBDA COND
;; SIMPLIFIED BUG FIXED VERSION OF JOHN MCCARTHY PAPER
;; NOTE: ((EQ (CAR E) ()) (QUOTE *UNDEFINED)) CAN HELP
;; NOTE: ((EQ (CAR E) (QUOTE LAMBDA)) E) IS NICE
((LAMBDA (ASSOC EVCON PAIRLIS EVLIS APPLY EVAL)
   (EVAL (QUOTE ((LAMBDA (FF X) (FF X))
                 (QUOTE (LAMBDA (X)
                          (COND ((ATOM X) X)
                                ((QUOTE T) (FF (CAR X))))))
                 (QUOTE ((A) B C))))
         ()))
 (QUOTE (LAMBDA (X Y)
          (COND ((EQ Y ()) ())
                ((EQ X (CAR (CAR Y)))
                       (CDR (CAR Y)))
                ((QUOTE T)
                 (ASSOC X (CDR Y))))))
 (QUOTE (LAMBDA (C A)
          (COND ((EVAL (CAR (CAR C)) A)
                 (EVAL (CAR (CDR (CAR C))) A))
                ((QUOTE T) (EVCON (CDR C) A)))))
 (QUOTE (LAMBDA (X Y A)
          (COND ((EQ X ()) A)
                ((QUOTE T) (CONS (CONS (CAR X) (CAR Y))
                                 (PAIRLIS (CDR X) (CDR Y) A))))))
 (QUOTE (LAMBDA (M A)
          (COND ((EQ M ()) ())
                ((QUOTE T) (CONS (EVAL (CAR M) A)
                                 (EVLIS (CDR M) A))))))
 (QUOTE (LAMBDA (FN X A)
          (COND
            ((ATOM FN)
             (COND ((EQ FN (QUOTE CAR))  (CAR  (CAR X)))
                   ((EQ FN (QUOTE CDR))  (CDR  (CAR X)))
                   ((EQ FN (QUOTE ATOM)) (ATOM (CAR X)))
                   ((EQ FN (QUOTE CONS)) (CONS (CAR X) (CAR (CDR X))))
                   ((EQ FN (QUOTE EQ))   (EQ   (CAR X) (CAR (CDR X))))
                   ((QUOTE T)            (APPLY (EVAL FN A) X A))))
            ((EQ (CAR FN) (QUOTE LAMBDA))
             (EVAL (CAR (CDR (CDR FN)))
                   (PAIRLIS (CAR (CDR FN)) X A))))))
 (QUOTE (LAMBDA (E A)
          (COND
            ((ATOM E) (ASSOC E A))
            ((ATOM (CAR E))
             (COND ((EQ (CAR E) (QUOTE QUOTE)) (CAR (CDR E)))
                   ((EQ (CAR E) (QUOTE COND)) (EVCON (CDR E) A))
                   ((QUOTE T) (APPLY (CAR E) (EVLIS (CDR E) A) A))))
            ((QUOTE T) (APPLY (CAR E) (EVLIS (CDR E) A) A))))))
