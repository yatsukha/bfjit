# increment loop counter at cell 0 to 40
# this takes around 1 to 2 seconds on a reasonable CPU
+++++ +++++
+++++ +++++
+++++ +++++
+++++ +++++

# n1 is at cell 1 set to 0
# n2 is at cell 2 set to 1
>> + <<

# n1 will be erased and n2 will be shifted to cell 1
# and in the process of shifting the n1 plus n2 will be in cell 2

[
  # decrement loop counter at cell 0
  -

  # move to cell 1
  >
  # erase while incrementing the cell 3
  [>> + << -]

  # move to cell 2; copy to 1 and increment 3
  >
  [<+ >>+ <-]

  # add cell 3 to 2 while incrementing cell 4
  # this extra step is required to preserve cell 3 value
  >
  [<+ >- >+<]

  # move to cell 4; cell 3 is zero and ready for next loops addition
  >

  # copy over cell 4 to cell 20 while erasing it
  [
    -

    >>>>
    >>>>
    >>>>
    >>>>

    +

    <<<<
    <<<<
    <<<<
    <<<<
  ]

  # move to cell 20
  >>>>
  >>>>
  >>>>
  >>>>

  # n divmod 10 to extract chars for printing
  [
    # set the divisor to 10 at offset 1
    >
    +++++ +++++
    <

    # divmod algorithm
    # input:
    # n
    # d
    # result:
    # 0
    # d minus n % d
    # n % d
    # n / d
    [->-[>+>>]>[+[-<+>]>+>>]<<<<<]


    # clear out cell at offset 1 since we dont need it
    > [-]
    # move cell at offset 2 to cell at minus 12 from this location (cell 10 plus i)
    > [<< <<<<< <<<<< + >> >>>>> >>>>> -]

    # increment to a printable value
    # important if modulo is 0
    << <<<<< <<<<<
      > +++ +++
      [< ++++ ++++ > -] # 48
      <
    >> >>>>> >>>>>

    # move cell at offset 3 to offset 1 as the input for next round of divmod
    # we move to offset 1 instead of 0 to keep moving towards right
    # otherwise we would overwrite existing chars that are to be printed
    > [<< + >> -]

    # go to starting cell at 20 plus i; where i is the current iteration
    # (since we keep moving towards right)
    # if n / d is 0 it will end here
    <<
  ]


  # find a non zero cell to the left
  # this will be the first char that we need to print
  +[<[>-]>[-<+>]<]<

  # while the cells are filled
  [
    . # output
    [-] # zero out
    < # go left
    
    # if we exhausted characters it will break here
  ]

  # newline print and zero out
  +++++ +++++
  .
  [-]

  # find a non zero cell to the left
  # this will be cell 2
  +[<[>-]>[-<+>]<]<
  
  # move to cell zero
  <<
]


# brief complexity analysis:
# n1 plus n2 to get the next fib is linear in n1 plus n2
# which for nth fibonnaci means exponential = 1(dot)6^n
# divmod algorithm is number of character times divmod complexity
# 
# number of chars: log(fib) aka log(1(dot)6^n) which is essentially just n
# divmod is polynomial so n^k
# and we calculate n fibonacci numbers
# so upper bound of total complexity is
#
# n * n ^ k or just polynomial n ^ k in other words it is O(divmod complexity)


# even this small program boils down to more llvm IR instructions
# than can fit in my scrollback buffer
# (and that is after the optimization passes)
# granted this is a naive mappinng from brainf*ck to llvm IR
# without anything smart in between
# such as brainf*ck specific optimization passes
# with some effort you could detect common patterns and optimize them out
