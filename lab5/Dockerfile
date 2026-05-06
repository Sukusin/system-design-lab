FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest AS build
WORKDIR /app
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j

FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest
WORKDIR /service
COPY --from=build /app/build/hotel_booking_hw05_userver /service/hotel_booking_hw05_userver
COPY static_config.yaml /service/static_config.yaml
COPY secure_data.json /service/secure_data.json
EXPOSE 8080
CMD ["/service/hotel_booking_hw05_userver", "-c", "/service/static_config.yaml"]
