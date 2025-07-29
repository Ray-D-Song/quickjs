function simple_add(n) {
  if (n <= 1) return n
  return n + 2
}

function test() {
  const iterations = 1000
  const testN = 5

  let total = 0
  for (let i = 0; i < iterations; i++) {
    total += simple_add(testN)
  }

  return total
}

for (let run = 0; run < 3; run++) {
  const start = Date.now()
  const result = test()
  const end = Date.now()

  print(`Run ${run + 1}: Result=${result}, Time=${end - start}ms`);
}