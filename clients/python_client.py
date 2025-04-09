import grpc
import data_pb2
import data_pb2_grpc

def run():
    channel = grpc.insecure_channel('localhost:50051')
    stub = data_pb2_grpc.DataPortalStub(channel)
    response = stub.SendData(data_pb2.DataRequest(id="42", payload="Hello from Python!"))
    print("Ack:", response.message)

if __name__ == '__main__':
    run()
