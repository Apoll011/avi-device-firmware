
idf_component_register(
    SRCS "animations.cpp"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES FastLED      # Private dependency, not exposed to others
)
