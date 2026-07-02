# TODO
- pointer semantics and syntax
    - andThen and transfrom
- defer!
    - migrate to not using functional?
- seperate files!
```cc
// pointer semantics
auto main() noexcept -> core::i32 {
    core::memptr<core::string<>> name;
    std::allocator<decltype(name)::char_t> char_alloc;
    name.allocate(std::allocator<core::string<>>());

    core::construct_at(name).append(char_alloc, "[CA]");
    defer _ = [=]{ 
        name.deallocate(char_alloc);
        core::destroy_at(name);
    };
}
```
