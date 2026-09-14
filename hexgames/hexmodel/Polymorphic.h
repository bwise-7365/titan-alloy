// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A value that holds one object of a class derived from Base and copies it by cloning, so that a
// Position holding a game's own types (its state, its obligations) still copies as a value for a
// fork. Base declares `virtual std::unique_ptr<Base> clone() const = 0;`; a derived class gets it
// from Cloneable<Derived, Base>. The one checked accessor, as<T>(), throws std::invalid_argument
// naming what was wanted when the value is empty or holds another type.
// ----------------------------------------------
#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace HexModel {

  template <class Derived, class Base>
  class Cloneable : public Base {
  public:
    std::unique_ptr<Base>
    clone() const override
    {
      return std::make_unique<Derived>(static_cast<const Derived&>(*this));
    }
  };

  template <class Base>
  class Polymorphic {
  public:
    Polymorphic() = default;
    explicit Polymorphic(std::unique_ptr<Base> held) : held_(std::move(held))
    {
      if (nullptr == held_) {
        throw std::invalid_argument("Polymorphic: constructed from an empty pointer");
      }
    }
    Polymorphic(const Polymorphic& other) : held_(copyOf(other)) {}
    Polymorphic(Polymorphic&&) noexcept = default;
    Polymorphic&
    operator=(const Polymorphic& other)
    {
      if (this != &other) {
        held_ = copyOf(other);
      }
      return *this;
    }
    Polymorphic& operator=(Polymorphic&&) noexcept = default;
    ~Polymorphic() = default;

    bool holdsP() const { return nullptr != held_; }

    // The held object as its base; throws when empty.
    const Base&
    base() const
    {
      if (nullptr == held_) {
        throw std::invalid_argument("Polymorphic: nothing is held");
      }
      return *held_;
    }

    template <class T>
    const T&
    as(const char* what) const
    {
      const T* typed = dynamic_cast<const T*>(held_.get());
      if (nullptr == typed) {
        throw std::invalid_argument(std::string(what) + (held_ ? " holds another type" : " is absent"));
      }
      return *typed;
    }

    template <class T>
    T&
    as(const char* what)
    {
      T* typed = dynamic_cast<T*>(held_.get());
      if (nullptr == typed) {
        throw std::invalid_argument(std::string(what) + (held_ ? " holds another type" : " is absent"));
      }
      return *typed;
    }

  private:
    static std::unique_ptr<Base>
    copyOf(const Polymorphic& other)
    {
      if (nullptr == other.held_) {
        return nullptr;
      }
      return other.held_->clone();
    }

    std::unique_ptr<Base> held_;
  };

  // Builds a Polymorphic<Base> holding a T made from the arguments.
  template <class Base, class T, class... Args>
  Polymorphic<Base>
  makePolymorphic(Args&&... args)
  {
    return Polymorphic<Base>(std::make_unique<T>(std::forward<Args>(args)...));
  }

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
