function fib(n) {
  if (n <= 1) return n
  return fib(n - 1) + fib(n - 2)
}

function test() {
  const iterations = 1000
  const fibN = 20

  let total = 0
  for (let i = 0; i < iterations; i++) {
    total += fib(fibN)
  }

  return total
}

for (let run = 0; run < 5; run++) {
  const start = Date.now()
  const result = test()
  const end = Date.now()

  print(`Run ${run + 1}: Result=${result}, Time=${end - start}ms`);
}