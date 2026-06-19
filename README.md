# Object-Oriented C

### How to get classes, inheritance, and polymorphism out of a language that has none of them

C doesn't have objects. There is no `class` keyword, no `new`, no `this`, nothing called a
method, and certainly no `virtual`. And yet a huge amount of the object-oriented software you
rely on every day is written in plain C: the Linux kernel's virtual filesystem, GTK and the
whole GObject type system, the Python interpreter, OpenSSL, libcurl. None of those people were
confused about what language they were using. They just understood something that gets lost in
the `class`-vs-`struct` debate: object orientation is a way of organizing code, and the keywords
are only sugar.

This repository is a small, deliberately complete example of that idea. A handful of shapes, a
base type they all share, a collection that holds them, and a `main` that treats circles and
rectangles as if they were interchangeable. Roughly two hundred lines of C, no comments in the
source on purpose, and every one of the four classic pillars of OOP implemented by hand so you
can see the gears turning. This document is the long version of what the code is doing and why.

If you want to run it first and read later:

```bash
make run
```

---

## What an object actually is

Strip away the syntax and an object is two things stuck together: some **state** and some
**behavior** that operates on that state. A bank account has a balance (state) and knows how to
deposit and withdraw (behavior). The deposit function isn't floating in space; it belongs to the
account, and it implicitly knows which account it's working on.

C gives you both halves directly. State is a `struct`. Behavior is a function. The only thing the
language won't do for you is *glue* the function to the struct and hide the `this` pointer, so
you do that part yourself. Once you accept that you'll be passing the object in by hand as the
first argument, the rest is just discipline.

Here's the simplest possible "object" before we add any machinery:

```c
typedef struct {
    double balance;
} Account;

void account_deposit(Account *self, double amount) {
    self->balance += amount;
}
```

`self` is the only thing C++ hides from you when you write `account.deposit(amount)`. The
convention of naming the first parameter `self` and prefixing every function with the type name
(`account_`) is doing the work that a class declaration would do for free. You'll see exactly
this pattern everywhere in the real code: `shape_area(shape)`, `shape_collection_add(shapes, c)`.
It reads as `shape.area()` and `shapes.add(c)` once your eyes adjust.

That's the whole trick, conceptually. Everything below is about making it scale: how to hide the
struct's internals, how to make one type extend another, and how to make a single function call
do different things depending on what's really behind the pointer.

---

## Pillar one: abstraction, or splitting the "what" from the "how"

A header file is an interface. The `.c` file behind it is the implementation. That split is the
oldest form of abstraction in the language, and C programmers were doing it for decades before
anyone wrote it on a slide.

Look at [`include/shape.h`](include/shape.h). It tells you that a shape has an area, a perimeter,
a way to describe itself, and a way to be destroyed:

```c
double shape_area(const Shape *self);
double shape_perimeter(const Shape *self);
void shape_describe(const Shape *self);
void shape_destroy(Shape *self);
```

Nothing in that list says how the area is computed. It can't, because circles and rectangles
compute it differently, and the header has no idea which one you're holding. That's the point.
A caller includes `shape.h`, links against the compiled object files, and never needs to open a
single `.c` file. The interface is a promise; the implementation is somebody else's problem.

This is also where you should start designing any C "class": write the header first, as the set
of operations you wish you had, and only then go figure out how to satisfy them.

---

## Pillar two: encapsulation, or making "private" real

Abstraction hides *how* something works. Encapsulation hides the *data* so nobody can reach in
and corrupt it. In a class-based language you write `private` and the compiler slaps your hand.
C has no `private`, but it has something almost as good: a type whose definition the compiler
can see in one translation unit and not in the others. This is the **opaque pointer**, and it's
the cleanest thing in this whole repository.

Open [`include/shape_collection.h`](include/shape_collection.h). The entire type is this:

```c
typedef struct ShapeCollection ShapeCollection;
```

A name and nothing else. There are no fields here. Anyone who includes this header knows that a
`ShapeCollection` exists and that they can hold a `ShapeCollection *`, but they cannot create one
on the stack, cannot read `.size`, cannot touch the internal array, because as far as their
compiler is concerned the struct has no members. The compiler doesn't even know how big it is.

The actual layout lives in [`src/shape_collection.c`](src/shape_collection.c), invisible to the
outside world:

```c
struct ShapeCollection {
    Shape **items;
    size_t size;
    size_t capacity;
};
```

Those three fields are as private as anything in Java. The only way to do anything with a
collection is to go through the functions the header exposes: `shape_collection_add`,
`shape_collection_size`, `shape_collection_at`, and so on. If you decide tomorrow to swap the
dynamic array for a linked list, no caller breaks, because no caller ever knew there was an array.

It's worth noticing the deliberate contrast in this project. `ShapeCollection` is fully opaque.
`Shape`, on the other hand, exposes its struct right there in the header:

```c
struct Shape {
    const ShapeOperations *operations;
    const char *type_name;
};
```

That's not an inconsistency, it's a tradeoff. Inheritance, which we get to next, requires the
derived types to physically embed the base struct, and you can't embed a type whose size you
can't see. So `Shape` pays for inheritance with reduced privacy, while `ShapeCollection`, which
nobody needs to extend, keeps everything locked down. Knowing *when* to open up the struct and
when to seal it is most of the craft here.

---

## Constructors, destructors, and who owns what

Before inheritance, a word about lifetimes, because C will not manage them for you and getting
this wrong is how the analogy turns into a pile of memory leaks.

There is no `new` and no garbage collector. A constructor in this codebase is just a function
that allocates, initializes, and hands back a pointer. Look at the bottom of
[`src/circle.c`](src/circle.c):

```c
Shape *circle_create(double radius) {
    Circle *circle = malloc(sizeof(*circle));
    if (circle == NULL) {
        return NULL;
    }
    shape_init(&circle->base, &circle_operations, "Circle");
    circle->radius = radius;
    return &circle->base;
}
```

Three things are happening. It grabs heap memory and checks that it actually got it (every single
allocation in this project is checked, because an unchecked `malloc` is a bug waiting for a bad
day). It wires up the base part of the object through `shape_init`. Then it fills in the fields
that are specific to a circle. The return type is `Shape *`, not `Circle *`, and that's a design
choice we'll come back to: from the caller's point of view this is a shape, full stop.

The destructor is the mirror image. `shape_destroy` eventually calls `free`, and the rule the
whole program lives by is: **whoever allocates is responsible, until ownership is explicitly
handed off.** In `main` the shapes are created and immediately handed to the collection. From
that moment the collection owns them, and when you call `shape_collection_destroy` it walks its
array, destroys every shape it's holding, and then frees its own storage:

```c
void shape_collection_destroy(ShapeCollection *self) {
    if (self == NULL) {
        return;
    }
    for (size_t index = 0; index < self->size; index++) {
        shape_destroy(self->items[index]);
    }
    free(self->items);
    free(self);
}
```

One call at the end of `main` tears down everything cleanly. That's not an accident; it's the
ownership model being deliberate about who frees whom. In a language with destructors this would
happen for you when the object goes out of scope. In C it happens because you wrote it down.

---

## Pillar three: inheritance, the first-member trick

Here is where people assume C gives up, and here is where it quietly doesn't.

C++ inheritance, underneath all the syntax, lays the base class's fields at the *start* of the
derived object's memory and then appends the new fields after them. A `Circle` is a `Shape` with
extra stuff bolted on the end. You can do precisely that by hand, and the standard even blesses
it. From [`src/circle.c`](src/circle.c):

```c
typedef struct Circle {
    Shape base;
    double radius;
} Circle;
```

`Shape base` is the first member. Not the second, not buried in the middle. First, on purpose.
The C standard guarantees that a pointer to a struct, suitably converted, points to its initial
member, and vice versa. So the address of a `Circle` is also the address of its `base`. The two
pointers are numerically identical; only their types differ.

That single guarantee is what makes the whole hierarchy work. When `circle_create` returns
`&circle->base`, it's handing back a `Shape *` that points at the front of a real `Circle` sitting
in memory. The caller sees a shape. The bytes after the shape, the radius, are still there; the
caller just doesn't know about them. This is an **upcast**, and in C it costs nothing: no
conversion, no runtime check, just a different label on the same address.

The reverse trip is the **downcast**, and you see it at the top of every method in `circle.c`:

```c
static double circle_area(const Shape *self) {
    const Circle *circle = (const Circle *)self;
    return PI * circle->radius * circle->radius;
}
```

The dispatcher handed this function a `Shape *`, but the function knows (because it's only ever
installed for circles, as we're about to see) that the shape is really the front of a circle. So
it casts the pointer back to `Circle *` and now the radius is reachable again. The cast doesn't
move anything or copy anything. It just tells the compiler to start interpreting those bytes as a
`Circle` instead of a `Shape`.

[`src/rectangle.c`](src/rectangle.c) does the identical dance with a `width` and a `height`. Add a
triangle tomorrow and it's the same shape of code: embed `Shape base` first, add your own fields,
cast in your methods. That uniformity is the whole reason the pattern scales.

A real warning, because this is the sharpest edge in the language: the first-member guarantee is
exactly that, *first member*. The moment you reorder the struct and `base` is no longer at offset
zero, the upcast silently starts handing out a pointer into the middle of your object, and every
downcast reads garbage. There's no compiler error. The convention is load-bearing, and the
discipline of always putting the base first is the price of admission.

---

## Pillar four: polymorphism, building a vtable by hand

Inheritance lets a circle *be* a shape. Polymorphism is the payoff: calling `shape_area` on
something you only know as a `Shape *` and having the *circle's* area calculation run, without
the caller knowing or caring that it's a circle. This is the part that feels like magic in
object-oriented languages, and it's the part C makes you build explicitly, which is exactly why
it's worth building once by hand.

The mechanism is a table of function pointers. C++ calls it a vtable and hides it inside every
object with a virtual method; here it's right out in the open. In
[`include/shape.h`](include/shape.h):

```c
typedef struct ShapeOperations {
    double (*area)(const Shape *self);
    double (*perimeter)(const Shape *self);
    void (*describe)(const Shape *self);
    void (*destroy)(Shape *self);
} ShapeOperations;
```

Four fields, each one a pointer to a function. This struct *is* the set of virtual methods. It
doesn't say what any of them do; it only fixes their shapes. Each concrete type then fills in one
of these tables with its own implementations, declares it `static const` so there's exactly one
copy shared by every instance of that type, and never touches it again. From `circle.c`:

```c
static const ShapeOperations circle_operations = {
    circle_area,
    circle_perimeter,
    circle_describe,
    circle_destroy,
};
```

Every `Shape` carries a pointer to one of these tables, set once in the constructor via
`shape_init`. A circle's `operations` points at `circle_operations`; a rectangle's points at
`rectangle_operations`. The object literally knows which functions are its own.

Now the dispatch. The public functions in [`src/shape.c`](src/shape.c) don't compute anything.
They look up the right function in the table and forward the call:

```c
double shape_area(const Shape *self) {
    return self->operations->area(self);
}
```

Read that slowly, because it's the entire idea in one line. Take the shape. Follow its
`operations` pointer to its table. Pull out the `area` slot. Call it, passing the shape back in
as `self`. If `self` is really a circle, `operations` points at `circle_operations`, the `area`
slot holds `circle_area`, and the circle's formula runs. If it's a rectangle, the exact same line
of code runs `rectangle_area` instead. The decision happens at runtime, based on data inside the
object, and `shape_area` never branches on a type at all. There is no `switch`, no `if
type == CIRCLE`. That's **dynamic dispatch**, and it's what `virtual` compiles down to in C++.

You can trace a full call to convince yourself nothing is hidden:

1. `main` holds a `Shape *` it got from the collection. It calls `shape_describe(s)`.
2. `shape_describe` does `s->operations->describe(s)`. Say `s` is a circle, so `operations`
   points at `circle_operations` and `describe` is `circle_describe`.
3. `circle_describe` casts `s` back to `Circle *`, reads the radius, and to print the area it
   calls `shape_area(s)` — which dispatches *again* through the table and lands in `circle_area`.
4. Numbers come back up the stack and get printed.

Every layer is an ordinary function call you could set a breakpoint on. The "magic" is just one
extra pointer hop through a table, which is exactly the cost a C++ virtual call pays too.

---

## Bringing it together

[`src/main.c`](src/main.c) is where the abstraction earns its keep. It never mentions a circle's
radius or a rectangle's width. It builds a collection, throws differently-typed shapes into it,
and then operates on all of them through the single `Shape` interface:

```c
shape_collection_add(shapes, circle_create(2.0));
shape_collection_add(shapes, rectangle_create(3.0, 4.0));
shape_collection_add(shapes, circle_create(1.5));
shape_collection_add(shapes, rectangle_create(5.0, 5.0));

shape_collection_describe_all(shapes);
printf("Total area: %.2f\n", shape_collection_total_area(shapes));
```

`shape_collection_total_area` loops over its array calling `shape_area` on each element, and each
call quietly routes itself to the right formula. Mixed types, one loop, no type tags. Add a
triangle to the project and this code doesn't change by a single character — it'll sum the
triangle's area right alongside the rest the moment you drop one in the collection. That property,
old code working unmodified with new types, is the open/closed principle, and it falls out of the
vtable design for free.

Run it and you get:

```
Collection holds 4 shapes:
[0] Circle(radius=2.00) -> area=12.57, perimeter=12.57
[1] Rectangle(width=3.00, height=4.00) -> area=12.00, perimeter=14.00
[2] Circle(radius=1.50) -> area=7.07, perimeter=9.42
[3] Rectangle(width=5.00, height=5.00) -> area=25.00, perimeter=20.00
Total area: 56.63
```

---

## The map, file by file

```
include/
  shape.h               the Shape struct, the vtable type, the public operations
  circle.h              one line: how to construct a Circle
  rectangle.h           one line: how to construct a Rectangle
  shape_collection.h    the opaque collection type and its operations
src/
  shape.c               the dispatchers that forward calls through the vtable
  circle.c              Circle's private struct, methods, vtable, constructor
  rectangle.c           the same, for rectangles
  shape_collection.c    the dynamic array hiding behind the opaque type
  main.c                a program that only ever speaks in Shapes
```

The headers are the contracts. The `.c` files are the secrets. If you only read the four headers
you'd understand how to *use* the whole system without knowing how any of it works, and that's
the abstraction doing its job.

---

## Where the illusion stops

Honesty matters more than enthusiasm, so here's what you're giving up by doing OOP this way
instead of reaching for an actual object-oriented language.

The compiler is not on your side. The downcast from `Shape *` to `Circle *` is an unchecked cast.
If you wire a circle's vtable into a rectangle's struct by mistake, nothing complains and you get
undefined behavior at runtime. The first-member rule is a convention the language enforces only by
giving you wrong answers when you break it.

Nothing is automatic. Constructors don't chain, destructors don't fire when something leaves
scope, and the vtable doesn't populate itself; you write `shape_init` into every constructor by
hand and you call `shape_destroy` yourself. Forget one and you leak. There's no `override`
keyword to catch a method signature that drifted out of sync with the vtable, either.

And the features taper off fast past this point. Single inheritance is clean; multiple inheritance
means embedding several base structs and the first-member trick only works for one of them, so the
others need pointer arithmetic to recover. There's no method overloading, because C has one
function per name. No generics worth the name. No exceptions, so error handling is return codes
and `NULL` checks all the way down, which is why you see them on every allocation here.

None of that makes the technique wrong. It makes it a tool with a known cost. When you're writing
a device driver, an interpreter, or a library that has to expose a C ABI to the entire world, this
is often exactly the right amount of object orientation, and it's why the pattern shows up in
nearly every serious C codebase under names like "ops structs" or "method tables." You're not
fighting the language. You're using it the way its biggest projects do.

---

## Make it yours

The fastest way to understand a pattern is to extend it, and this one is built to be extended.

Start by adding a `Triangle`. Copy the shape of `circle.c`: a struct with `Shape base` first and
a base and height after it, four `static` methods, a `static const ShapeOperations` table, and a
`triangle_create` constructor. Add a one-line `triangle.h`. Then add one to the collection in
`main.c` and watch the total area update without you touching a single existing function. If you
understood the vtable, this will take you ten minutes and prove the whole point.

After that: add a `clone` slot to the vtable and implement deep copies for each type. Add
`shape_collection_remove_at` and make sure ownership and freeing stay correct when an element
leaves in the middle. Give `Shape` a field the base methods actually use, like a name you can set,
and watch how the base "class" starts carrying shared state for everyone. Each of those pushes on
a different corner of the design, and each one teaches you something the prose here can only point
at.
