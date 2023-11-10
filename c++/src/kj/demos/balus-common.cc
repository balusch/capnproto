/*
 * Copyright(C) 2024
 */

#include <type_traits>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

#include <kj/exception.h>
#include <kj/common.h>
#include <kj/debug.h>
#include <kj/map.h>
#include <kj/string-tree.h>
#include <kj/one-of.h>
#include <kj/refcount.h>
#include <kj/string.h>
#include <kj/table.h>

class Widget : public kj::Refcounted {
public:
  Widget(kj::StringPtr name) : seq(next_seq++), name(kj::heapString(name)) {}
  KJ_DISALLOW_COPY_AND_MOVE(Widget);
  ~Widget() noexcept(false) override = default;

  const kj::String &getName() const { return name; }
  kj::uint getSeq() const { return seq; }

private:
  kj::uint seq;
  kj::String name;

  static thread_local kj::uint next_seq;
};

thread_local kj::uint Widget::next_seq = 1;

class Label {
public:
    Label(kj::StringPtr name) : seq(++global_seq), name(kj::heapString(name)) {}
    Label(const Label &label) { *this = label; }
    Label(Label &&label) { *this = kj::mv(label); }
    Label& operator=(const Label &label) {
      seq = ++global_seq;
      name = kj::heapString(label.name);
      return *this;
    }
    Label& operator=(Label &&label) {
      seq = label.seq;
      name = kj::mv(label.name);
      label.seq = 0;
      return *this;
    }

    // getter
    kj::uint getSeq() const { return seq; }
    const kj::String &getName() const { return name; }

    // required by kj::HashIndex
    kj::uint hashCode() const { return kj::hashCode(name); }
    bool operator==(const Label &label) const { return label.name == name; }

    // required by KJ_LOG
    kj::String toString() const { return kj::str(name, "-", seq); }

private:
    kj::uint seq;
    kj::String name;
    static thread_local kj::uint global_seq;
};

thread_local kj::uint Label::global_seq = 0;

int main(int argc, char **argv) {
  {
    kj::Own<Widget> w1 = kj::refcounted<Widget>("tom");
    KJ_LOG(WARNING, "before: is shared", w1->isShared());
    kj::Own<Widget> w2 = kj::addRef(*w1);
    KJ_LOG(WARNING, "after: is shared", w1->isShared());
  }

  {
    kj::Rc<Widget> w1 = kj::rc<Widget>("tom");
    KJ_LOG(WARNING, "before: is shared", w1->isShared());
    kj::Rc<Widget> w2 = w1.addRef();
    KJ_LOG(WARNING, "after 1: is shared", w1->isShared());
    kj::dtor(w2);
    KJ_LOG(WARNING, "after 2: is shared", w1->isShared());
  }

  {
    kj::Own<kj::RefcountedWrapper<Label>> todo = kj::refcountedWrapper<Label>("todo");
    kj::Own<Label> lb1 = todo->addWrappedRef();
    kj::Own<Label> lb2 = todo->addWrappedRef();
    KJ_ASSERT(lb1.get() == &todo->getWrapped());
    KJ_ASSERT(lb1.get() == lb2.get());
    kj::Own<kj::RefcountedWrapper<Label>> todo2 = kj::addRef(*todo);
    kj::dtor(todo);
    kj::dtor(lb1);
    kj::dtor(lb2);
  }

  {
    kj::Maybe<Widget> maybeW1 = kj::none;
    KJ_IF_SOME(w1, maybeW1) {
      KJ_LOG(WARNING, "name", w1.getName(), "seq", w1.getSeq());
    } else {
      KJ_LOG(WARNING, "maybeW1 is empty");
    }
  }

  {
    kj::OneOf<kj::String, Widget> oneOf = kj::heapString("Hello");
    KJ_SWITCH_ONEOF(oneOf) {
      KJ_CASE_ONEOF(s, kj::String) {
        KJ_LOG(WARNING, "oneOf is a kj::String", s);
      }
      KJ_CASE_ONEOF(w, Widget) {
        KJ_LOG(WARNING, "oneOf is a Widget", w.getName(), w.getSeq());
      }
    }
  }

  {
    {
      kj::HashSet<Label> labels;
      labels.insert(Label("tom"));
      labels.upsert(Label("tom"), [](Label &old, Label new_) {
        KJ_LOG(WARNING, old.getName(), old.getSeq(), new_.getName(), new_.getSeq());
      });
      labels.findOrCreate(Label("jerry"), []() { return Label("tom"); });
      labels.contains(Label("tom"));
    }

    {
      kj::TreeMap<kj::String, Label> labels;

      for (auto i: kj::zeroTo(15)) {
        auto key = kj::str("tom", i);
        Label label(key);
        labels.insert(kj::mv(key), kj::mv(label));
      }

      labels.erase(kj::heapString("tom0"));
      labels.erase(kj::heapString("tom1"));

      for (auto i: kj::zeroTo(17)) {
        auto key = kj::str("tom", i);
        labels.erase(kj::mv(key));
      }

#if 0
      for (kj::uint i = 1; i < 102400; ++i) {
        int turn = rand() % 2;
        switch (turn) {
          case 0:
          case 1: {
            auto key = kj::str("tom", i);
            Label label(key);
            labels.insert(kj::mv(key), kj::mv(label));
            break;
          }
          case 2: {
            auto key = kj::str("tom", rand()%i);
            labels.erase(kj::mv(key));
            break;
          }
          default:
            KJ_UNREACHABLE
        }
      }

      for (auto it = labels.begin(); it != labels.end(); ++it) {
        KJ_LOG(WARNING, it->key, it->value);
      }
#endif
    }

    {
      kj::TreeSet<Label> labels;
    }

    {
      kj::TreeMap<kj::String, Label> labels;
    }
  }

  {
    std::set<int> iset;
    std::map<int, int> imap;

    std::unordered_set<int> ioset;
    std::unordered_map<int, int> iomap;
  }

  { // polymorphism
    class Bar { int i; virtual void hello() {} };
    class Baz { int j; virtual void world() {} };
    class Foo : public Bar, public Baz {};

    kj::Own<Foo> foo = kj::heap<Foo>();
    kj::Own<Bar> bar = kj::mv(foo);
  }

  { // non-polymorphism
    class Bar { int i; };
    class Baz { int j; };
    class Foo : public Bar, public Baz {};

#if 0
    kj::Own<Foo> foo = kj::heap<Foo>();
    kj::Own<Bar> bar = kj::mv(foo); // WON'T compile!!!
#else
    Foo foo;
    Bar bar = dynamic_cast<Bar&>(foo);
#endif
  }

  {
    // A 为非多态类型
    class A {};
    // B 为多态类型
    class B { public: virtual ~B(){} };
    // D 为非多态类型
    class D: public A {};
    // E 为非多态类型
    class E : private A {};
    // F 为多态类型
    class F : private B {};

    KJ_LOG(WARNING,
           std::is_polymorphic_v<A>,
           std::is_polymorphic_v<B>,
           std::is_polymorphic_v<D>,
           std::is_polymorphic_v<E>,
           std::is_polymorphic_v<F>);
  }

  {
    auto cat = kj::heapString("cats", sizeof("cats")-1);
    kj::StringTree st1 = kj::strTree("I'm ", 13, " years old", " and I love ", cat);
    st1.visit([](kj::ArrayPtr<const char> sequence) {
      KJ_LOG(WARNING, sequence);
    });

    kj::StringTree st = kj::strTree("Hello! ", kj::mv(st1), " Nice to meet you all!");
    auto str = st.flatten();
    KJ_LOG(WARNING, st);

    kj::String result = kj::heapString(st.size());
    st.flattenTo(result.begin(), result.end());

    st.visit([](kj::ArrayPtr<const char> sequence) {
      KJ_LOG(WARNING, sequence);
    });
  }

  return 0;
}