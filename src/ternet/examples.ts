export interface ExampleItem {
  id: string;
  name: string;
  category: string;
  description: string;
  code: string;
}

export const EXAMPLES: ExampleItem[] = [
  {
    id: 'oop-polymorphism',
    name: 'OOP & Virtual Dispatch',
    category: 'Object Oriented',
    description: 'Classes, inheritance, virtual and override methods with polymorphic dispatch.',
    code: `class Animal {
  virtual fn speak() {
    print("Animal makes a sound")
  }
}

class Dog extends Animal {
  override fn speak() {
    print("Dog: Woof!")
  }
}

class Cat extends Animal {
  override fn speak() {
    print("Cat: Meow!")
  }
}

fn main() {
  let animal: Animal = Dog()
  animal.speak()

  let kitty: Animal = Cat()
  kitty.speak()
}
`,
  },
  {
    id: 'traits-interfaces',
    name: 'Traits & Interfaces',
    category: 'Types & OOP',
    description: 'Trait declarations and implementations with static validation.',
    code: `trait Drawable {
  fn draw()
}

trait Resizable {
  fn resize(scale: int)
}

class Player implements Drawable, Resizable {
  let name: String
  let size: int

  fn init(name: String) {
    this.name = name
    this.size = 100
  }

  fn draw() {
    print("Drawing player: " + this.name)
  }

  fn resize(scale: int) {
    this.size = this.size * scale
    print("New size: " + this.size)
  }
}

fn main() {
  let p = Player("Hero")
  p.draw()
  p.resize(2)
}
`,
  },
  {
    id: 'generics',
    name: 'Generics & Containers',
    category: 'Generics',
    description: 'Generic Box container with type parameter instantiation.',
    code: `class Box<T> {
  let value: T

  fn init(val: T) {
    this.value = val
  }

  fn get() -> T {
    return this.value
  }
}

fn identity<T>(x: T) -> T {
  return x
}

fn main() {
  let b = Box(42)
  print("Box value: " + b.get())

  let idStr = identity("Ternet Language")
  print("Identity result: " + idStr)
}
`,
  },
  {
    id: 'lists-tuples',
    name: 'Lists & Tuples',
    category: 'Collections',
    description: 'Dynamic typed lists with mutation methods, and typed tuples with index access.',
    code: `fn main() {
  // Lists
  let numbers: List<int> = [10, 20, 30]
  numbers.append(40)
  numbers.append(50)
  print("List length: " + numbers.len())
  print("First item: " + numbers[0])
  print("Popped item: " + numbers.pop())

  // Tuples
  let user: (String, int, bool) = ("Ajmal", 15, true)
  let name = user.0
  let age = user.1
  let active = user.2

  print("User: " + name + ", Age: " + age + ", Active: " + active)
}
`,
  },
  {
    id: 'option-result',
    name: 'Option & Result Types',
    category: 'Functional',
    description: 'Algebraic Option<T> and Result<T, E> data types with unwrap and verification.',
    code: `fn divide(a: int, b: int) -> Result<int, String> {
  if b == 0 {
    return err("Division by zero")
  }
  return ok(a / b)
}

fn find_user(id: int) -> Option<String> {
  if id == 1 {
    return some("Alice")
  }
  return none()
}

fn main() {
  let res = divide(100, 4)
  if is_ok(res) {
    print("Result ok: " + unwrap(res))
  }

  let opt = find_user(1)
  if is_some(opt) {
    print("Found user: " + unwrap(opt))
  }
}
`,
  },
  {
    id: 'exceptions',
    name: 'Exception Handling',
    category: 'Control Flow',
    description: 'Try, catch with error bindings, finally blocks, and throw expressions.',
    code: `fn risky_operation(should_fail: bool) {
  if should_fail {
    throw "Database connection timed out"
  }
  print("Operation succeeded")
}

fn main() {
  try {
    print("Starting risky task...")
    risky_operation(true)
  } catch (err) {
    print("Caught exception: " + err)
  } finally {
    print("Cleanup executed in finally block.")
  }
}
`,
  },
  {
    id: 'fibonacci-benchmark',
    name: 'Fibonacci & Loops',
    category: 'Algorithms',
    description: 'Recursive and iterative Fibonacci sequence computation.',
    code: `fn fib_iter(n: int) -> int {
  if n <= 1 {
    return n
  }
  let a = 0
  let b = 1
  let i = 2
  while i <= n {
    let temp = a + b
    a = b
    b = temp
    i = i + 1
  }
  return b
}

fn main() {
  print("Computing Fibonacci numbers:")
  for n in [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10] {
    print("fib(" + n + ") = " + fib_iter(n))
  }
}
`,
  },
];
