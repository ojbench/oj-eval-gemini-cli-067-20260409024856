#include <iostream>
#include <optional>
#include <stdexcept>

class RefCellError : public std::runtime_error {
public:
    explicit RefCellError(const std::string& message) : std::runtime_error(message) {}
    virtual ~RefCellError() = default;
};

class BorrowError : public RefCellError {
public:
    explicit BorrowError(const std::string& message) : RefCellError(message) {}
};

class BorrowMutError : public RefCellError {
public:
    explicit BorrowMutError(const std::string& message) : RefCellError(message) {}
};

class DestructionError : public RefCellError {
public:
    explicit DestructionError(const std::string& message) : RefCellError(message) {}
};

template <typename T>
class RefCell {
private:
    T value;
    mutable int borrow_count = 0;

public:
    class Ref;
    class RefMut;

    explicit RefCell(const T& initial_value) : value(initial_value), borrow_count(0) {}
    explicit RefCell(T&& initial_value) : value(std::move(initial_value)), borrow_count(0) {}

    RefCell(const RefCell&) = delete;
    RefCell& operator=(const RefCell&) = delete;
    RefCell(RefCell&&) = delete;
    RefCell& operator=(RefCell&&) = delete;

    Ref borrow() const {
        if (borrow_count < 0) {
            throw BorrowError("Already mutably borrowed");
        }
        borrow_count++;
        return Ref(this);
    }

    std::optional<Ref> try_borrow() const {
        if (borrow_count < 0) {
            return std::nullopt;
        }
        borrow_count++;
        return Ref(this);
    }

    RefMut borrow_mut() {
        if (borrow_count != 0) {
            throw BorrowMutError("Already borrowed");
        }
        borrow_count = -1;
        return RefMut(this);
    }

    std::optional<RefMut> try_borrow_mut() {
        if (borrow_count != 0) {
            return std::nullopt;
        }
        borrow_count = -1;
        return RefMut(this);
    }

    class Ref {
    private:
        const RefCell* cell;

    public:
        Ref(const RefCell* c) : cell(c) {}
        Ref() : cell(nullptr) {}

        ~Ref() {
            if (cell) {
                cell->borrow_count--;
            }
        }

        const T& operator*() const {
            return cell->value;
        }

        const T* operator->() const {
            return &cell->value;
        }

        Ref(const Ref& other) : cell(other.cell) {
            if (cell) {
                cell->borrow_count++;
            }
        }

        Ref& operator=(const Ref& other) {
            if (this != &other) {
                if (cell) {
                    cell->borrow_count--;
                }
                cell = other.cell;
                if (cell) {
                    cell->borrow_count++;
                }
            }
            return *this;
        }

        Ref(Ref&& other) noexcept : cell(other.cell) {
            other.cell = nullptr;
        }

        Ref& operator=(Ref&& other) noexcept {
            if (this != &other) {
                if (cell) {
                    cell->borrow_count--;
                }
                cell = other.cell;
                other.cell = nullptr;
            }
            return *this;
        }
    };

    class RefMut {
    private:
        RefCell* cell;

    public:
        RefMut(RefCell* c) : cell(c) {}
        RefMut() : cell(nullptr) {}

        ~RefMut() {
            if (cell) {
                cell->borrow_count = 0;
            }
        }

        T& operator*() {
            return cell->value;
        }

        T* operator->() {
            return &cell->value;
        }

        RefMut(const RefMut&) = delete;
        RefMut& operator=(const RefMut&) = delete;

        RefMut(RefMut&& other) noexcept : cell(other.cell) {
            other.cell = nullptr;
        }

        RefMut& operator=(RefMut&& other) noexcept {
            if (this != &other) {
                if (cell) {
                    cell->borrow_count = 0;
                }
                cell = other.cell;
                other.cell = nullptr;
            }
            return *this;
        }
    };

    ~RefCell() noexcept(false) {
        if (borrow_count != 0) {
            throw DestructionError("Still borrowed when destructed");
        }
    }
};

