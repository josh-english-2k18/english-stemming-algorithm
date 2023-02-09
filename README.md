# English Stemming Algorithm - reference implementation in C
### Joshua S. English, 2003

## Abstract
This is an abstract of the English Stemming Algorithm Whitepaper authored by Joshua S. English in 2003. The algorithm describes a methodology for reducing the morphological variation of a given set of tokens, comprising words from a specified language, through conflation. This family of algorithms is commonly referred to as 'stemming', and is often used in information-retrieval applications for performance enhancement.

The full version of the Whitepaper may be found [here](http://www.xaede.net/blog/2009/06/14/english-stemming-algorithm-2003/).

## Introduction
The English Stemming Algorithm (ESA) is designed to iteratively apply a set of rules to reduce any given word to a common base by matching zero or more suffix strings against word-endings. Each rule has the capability to remove, replace, extend or otherwise alter the supplied word, and the order in which the ESA rules are defined is significant due to methodology where rules may be devised to act upon the results produced by previously applied rules, which which are always applied in the order in which they are defined.

The rule format used by ESA is comprised of four parts, which define the suffix to be matched, if the rule may be applied to a token which has already bee altered by another rule and finally the number of characters to remove, alter or append.

## Reference Implementation
The ESA reference implementation was structured such that the set of rules used by the algorithm were identical to the morphological conflation utilized by the Porter Stemming Algorithm (Porter, M.F, 1980). This was done to both ensure that the reference implementation was working correctly by comparing its output against that of the Porter algorithm, but also provided a framework for research efforts such as performance comparisons, etc. The algorithms were exhaustively tested against a database comprising the most commonly-used words in the English language, and it was found that ESA was, on average, 25% faster.

## Related Algorithm Comparisons
It was noted during the design phase that ESA, while exhibiting certain similarities to the Paice/Husk (i.e. the Lancaster Algorithms) (Paice, C; Husk, G; 1980) in that it uses a rule-set, the ESA rule-set is far shorter, faster to execute, and more flexible given its extended and interdependent rule capability. The application of a single-step rule-application methodology allowed ESA to outperform Porter, while also providing for extensible functionality (including the application against other languages). The full whitepaper goes on to detail further comparisons against other stemming algorithms, with the conclusion that the simplicity, performance, extensibility, and the potential for applying it to other languages clearly indicates the superiority of its design compared to the other algorithms commonly in-use today.

## Run Example
Simply execute the included script to compile and run the example against the reference implementation:
```
$ ./buildExample.sh
Example output from ESA:

         ...token 'This' => transform => 'this'
         ...token 'is' => transform => 'is'
         ...token 'a' => transform => 'a'
         ...token 'test' => transform => 'test'
 ...
         ...token 'their' => transform => 'their'
         ...token 'proper' => transform => 'prop'
         ...token 'selves' => transform => 'selv'


...Example completed.
```
