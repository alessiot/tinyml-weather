//
//  ConnectModelView.swift
//  iosWeather
//
//  Created by Alessio Tamburro on 3/4/24.
//

import Foundation

final class ConnectModelView: ObservableObject {
    @Published var state = State.idle
    
    var useCase: PeripheralUseCaseProtocol
    let connectedPeripheral: Peripheral
    
    init(useCase: PeripheralUseCaseProtocol,
         connectedPeripheral: Peripheral) {
        self.useCase = useCase
        self.useCase.peripheral = connectedPeripheral
        self.connectedPeripheral = connectedPeripheral
        self.setCallbacks()
    }
    
    private func setCallbacks() {
        useCase.onPeripheralReady = { [weak self] in
            self?.state = .ready
        }
        
        useCase.onSensorReading = { [weak self] temperature, humidity, weather in
            self?.state = .sensorReading(temperature, humidity, weather)
        }
        
        /*useCase.onWriteLedState = { [weak self] value in
            self?.state = .ledState(value)
        }*/
        
        useCase.onError = { error in
            print("Error \(error)")
        }
    }
    
    
    func startNotifySensor() {
        useCase.notifySensor(true)
    }

    func stopNotifySensor() {
        useCase.notifySensor(false)
    }

    func readSensor() {
        useCase.readSensor()
    }

    /*
    func turnOnLed() {
        useCase.writeLedState(isOn: true)
    }
    
    func turnOffLed() {
        useCase.writeLedState(isOn: false)
    }*/
}

extension ConnectModelView {
    enum State {
        case idle
        case ready
        //case ledState(Bool)
        case sensorReading(Float, Float, UInt8)
    }
}
