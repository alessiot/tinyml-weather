//
//  PeripheralUseCase.swift
//  iosWeather
//
//  Created by Alessio Tamburro on 3/4/24.
//

import Foundation
import CoreBluetooth

protocol PeripheralUseCaseProtocol {

    var peripheral: Peripheral? { get set }

    typealias SensorReadingCallback = (Float, Float, UInt8) -> Void
    var onSensorReading: SensorReadingCallback? { get set }
        
    //var onWriteLedState: ((Bool) -> Void)? { get set }
    var onPeripheralReady: (() -> Void)? { get set }
    var onError: ((Error) -> Void)? { get set }


    //func writeLedState(isOn: Bool)
    func readSensor()
    func notifySensor(_ isOn: Bool)
}

class PeripheralUseCase: NSObject, PeripheralUseCaseProtocol {
    
    var peripheral: Peripheral? {
        didSet {
            self.peripheral?.cbPeripheral?.delegate = self
            discoverServices()
        }
    }
    
    var cbPeripheral: CBPeripheral? {
        peripheral?.cbPeripheral
    }
    
    //var onWriteLedState: ((Bool) -> Void)?
    
    typealias SensorReadingCallback = (Float, Float, UInt8) -> Void
    var onSensorReading: SensorReadingCallback?
    var onPeripheralReady: (() -> Void)?
    var onError: ((Error) -> Void)?
    
    var discoveredServices = [CBUUID : CBService]()
    var discoveredCharacteristics = [CBUUID : CBCharacteristic]()
    
    func discoverServices() {
        //cbPeripheral?.discoverServices([UUIDs.modeloutputService, UUIDs.temperatureService])
        cbPeripheral?.discoverServices([UUIDs.weatherService])
    }
    /*
    func writeLedState(isOn: Bool) {
        guard let ledCharacteristic = discoveredCharacteristics[UUIDs.ledStatusCharacteristic] else {
            return
        }
        cbPeripheral?.writeValue(Data(isOn ? [0x01] : [0x00]), for: ledCharacteristic, type: .withResponse)
    }
    */
    func readSensor() {
        guard let temperatureCharacteristic = discoveredCharacteristics[UUIDs.temperatureCharacteristic],
              let humidityCharacteristic = discoveredCharacteristics[UUIDs.humidityCharacteristic],
              let weatherCharacteristic = discoveredCharacteristics[UUIDs.weatherCharacteristic] else {
            return
        }
        
        cbPeripheral?.readValue(for: temperatureCharacteristic)
        cbPeripheral?.readValue(for: humidityCharacteristic)
        cbPeripheral?.readValue(for: weatherCharacteristic)
    }

    func notifySensor(_ isOn: Bool) {
        guard let temperatureCharacteristic = discoveredCharacteristics[UUIDs.temperatureCharacteristic],
              let humidityCharacteristic = discoveredCharacteristics[UUIDs.humidityCharacteristic],
              let weatherCharacteristic = discoveredCharacteristics[UUIDs.weatherCharacteristic] else {
            return
        }
        
        cbPeripheral?.setNotifyValue(isOn, for: temperatureCharacteristic)
        cbPeripheral?.setNotifyValue(isOn, for: humidityCharacteristic)
        cbPeripheral?.setNotifyValue(isOn, for: weatherCharacteristic)
    }
    
    fileprivate func requiredCharacteristicUUIDs(for service: CBService) -> [CBUUID] {
        switch service.uuid {
        case UUIDs.weatherService:
            var requiredCharacteristics = [CBUUID]()
            if discoveredCharacteristics[UUIDs.temperatureCharacteristic] == nil {
                requiredCharacteristics.append(UUIDs.temperatureCharacteristic)
            }
            if discoveredCharacteristics[UUIDs.humidityCharacteristic] == nil {
                requiredCharacteristics.append(UUIDs.humidityCharacteristic)
            }
            if discoveredCharacteristics[UUIDs.weatherCharacteristic] == nil {
                requiredCharacteristics.append(UUIDs.weatherCharacteristic)
            }
            return requiredCharacteristics
        default:
            return []
        }
    }

}

extension PeripheralUseCase: CBPeripheralDelegate {
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let services = peripheral.services, error == nil else {
            return
        }
        for service in services {
            discoveredServices[service.uuid] = service
            let uuids = requiredCharacteristicUUIDs(for: service)
            guard !uuids.isEmpty else {
                return
            }
            peripheral.discoverCharacteristics(uuids, for: service)
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard let characteristics = service.characteristics else {
            return
        }
        for characteristic in characteristics {
            discoveredCharacteristics[characteristic.uuid] = characteristic
        }
        if discoveredCharacteristics[UUIDs.temperatureCharacteristic] != nil &&
            discoveredCharacteristics[UUIDs.weatherCharacteristic] != nil &&
            discoveredCharacteristics[UUIDs.humidityCharacteristic] != nil {
            onPeripheralReady?()
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        switch characteristic.uuid {
        case UUIDs.temperatureCharacteristic, UUIDs.humidityCharacteristic, UUIDs.weatherCharacteristic:
            guard let temperatureCharacteristic = discoveredCharacteristics[UUIDs.temperatureCharacteristic],
                  let humidityCharacteristic = discoveredCharacteristics[UUIDs.humidityCharacteristic],
                  let weatherCharacteristic = discoveredCharacteristics[UUIDs.weatherCharacteristic],
                  let temperatureData = temperatureCharacteristic.value,
                  let humidityData = humidityCharacteristic.value,
                  let weatherData = weatherCharacteristic.value else {
                return
            }

            // Assuming the data is a 32-bit float, you can interpret it as such
            let temperatureValue = temperatureData.withUnsafeBytes { (ptr: UnsafePointer<Float>) -> Float in
                return ptr.pointee
            }
            let humidityValue = humidityData.withUnsafeBytes { (ptr: UnsafePointer<Float>) -> Float in
                return ptr.pointee
            }
            let weatherValue = weatherData[0]

            // Invoke the callback with both values
            onSensorReading?(temperatureValue, humidityValue, weatherValue)
        default:
            fatalError()
        }
    }

}
